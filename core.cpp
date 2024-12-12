#include "core.h"

class CoreObject: public WaitSystem::Module, public Core {public:
    Core::Setup &setup;
    I4 seq, cant_send, buffer[50];
    struct pckt packet;
    struct measurement msmt[120000];
    bool can_send, have_settings, prepairing;

    Mgmt::Queue_job*                  mgmt_job;
    Mgmt::Queue_report*            mgmt_report;

    Global_setup::Queue_toSet*       setup_set;
    Global_setup::Queue_toSave*     setup_save;

    Packetizer::Queue_prx*        packetizer_rx;
    Packetizer::Queue_ptx*        packetizer_tx;
    Packetizer::Queue_psent*    packetizer_sent;


    CoreObject(WaitSystem* waitSystem, Core::Setup &setup): WaitSystem::Module(waitSystem)
            , setup(setup),can_send(false), prepairing(false), have_settings(false), seq(0), cant_send(0), mgmt_job(), mgmt_report(), packetizer_tx(), packetizer_rx(), packetizer_sent()
    {
        bzero(buffer, 50);
        module_debug = "CORE";
        flags |= evaluate_every_cycle;
    }
    WaitSystem::Queue timer;
    WaitSystem::Queue work;


    void attach_Global_setup(Global_setup::Queue_toSave* save, Global_setup::Queue_toSet* set){
        setup_set = set; setup_save = save;
        disable_wait(setup_set); disable_wait(setup_save);
        enable_wait(setup_set);
    }

    void attach_packetizer(Packetizer::Queue_prx* rx, Packetizer::Queue_ptx* tx, Packetizer::Queue_psent* sent){
        disable_wait(packetizer_tx); disable_wait(packetizer_rx); disable_wait(packetizer_sent);
        packetizer_tx = tx;
        packetizer_rx = rx;
        packetizer_sent = sent;
    }

    void attach_mgmt(Mgmt::Queue_job* job, Mgmt::Queue_report* report){
        disable_wait(mgmt_job); disable_wait(mgmt_report);
        mgmt_job = job; mgmt_report = report;
        enable_wait(mgmt_job);
    }

    int find_empty(){
        for(I2 i = 0; i < 50; i++){
            if(buffer[i] == 0) return i;
        }
    }

    void init(){
        have_settings = true;

        packetizer_rx->packet = mgmt_job->packet;
        packet = mgmt_job->packet;
        mgmt_job->clear();
        packetizer_rx->setReady();

        enable_wait(packetizer_rx); enable_wait(packetizer_tx);

        U64 work_duration = (packet.duration + 1) * 1000000000ULL;
        waitSystem->start_timer(&work, work_duration);
        waitSystem->enable_wait(this, &work);

        if(packet.is_server)init_server();
        else init_client();
    }

    void init_server(){
        print("I am server, awaiting for client for %i seconds", packet.duration);
    }

    void init_client(){
        enable_wait(packetizer_sent);
        U64 packet_duration = 1000000000ULL / packet.pckt_per_s;
        waitSystem->enable_wait(this, &timer);
        waitSystem->enable_wait(this, &work);
        waitSystem->start_timer(&timer, packet_duration);;
        print("I am client, working for %i seconds", packet.duration);
    }

    void report(){
        have_settings = false;
        work.clear();
        I4 packet_loss = 0;
        I64 rtt_avg = 0, rtt_max = 0, rtt_min = 10000000;
        for(I4 i = 0; i < packet.amount; i++){
            if(msmt[i].in_ts && msmt[i].up_ts && msmt[i].in_ts > msmt[i].up_ts){
                I4 delay = msmt[i].in_ts - msmt[i].up_ts;
                rtt_avg += delay;
                if(delay > rtt_max) rtt_max = delay;
                if(delay < rtt_min) rtt_min = delay;
            }
            else{
                packet_loss++;
            }
        }
        double avg_out = (double)(rtt_avg / packet.amount) / 1000000 / 2;
        double min_out = (double) rtt_min / 1000000 / 2;
        double max_out = (double) rtt_max / 1000000 / 2;
        float percent = (float)packet_loss * 100 / packet.amount;
        mgmt_report->report(avg_out, min_out, max_out, percent, packet_loss);
    }

    void evaluate() {
        while (WaitSystem::Queue* queue = enum_ready_queues()){
            if(!have_settings && queue==mgmt_job) init();
            if(queue == packetizer_tx) {
                can_send = true;
                packetizer_tx->clear();
            }
            if(packet.is_server){
                if(queue == &work) {
                    print("Server stopped!");
                    exit(EXIT_SUCCESS);
                }
                //СЕРВЕР
                if(can_send && cant_send != 0){
                    for(I2 i = 0; i < 50; i++){
                        if(buffer[i] != 0) {
                            I2 r = packetizer_tx->send(buffer[i]);
                            if(r > 0) buffer[i] = 0;
                        }
                    }
                }
                if(queue == packetizer_rx){
                    //СЕРВЕР ПРИНИМАЕТ
                    I4 sequence; U64 ts;
                    I2 r = packetizer_rx->recv(sequence, ts);
                    if(r > 0){
                        seq = sequence;
                        if(can_send){
                            //СЕРВЕР ОТПРАВЛЯЕТ
                            packetizer_tx->send(sequence);
                        }
                        else{
                            //СЕРВЕР НЕ МОЖЕТ ОТПРАВИТЬ И ЗАПОМИНАЕТ
                            I2 empty_index = find_empty();
                            buffer[empty_index] = sequence;
                            cant_send++;
                        }
                    }
                }
            }
            else{
                if(queue == &work) report();
                //КЛИЕНТ
                if(queue == packetizer_rx){
                    //КЛИЕНТ ПРИНИМАЕТ
                    I4 sequence; U64 ts;
                    I2 status = packetizer_rx->recv(sequence, ts);
                    if(status > 0 && !msmt[sequence].in_ts) msmt[sequence].in_ts = ts;
                }
                else if(queue == &timer && can_send){
                    //КЛИЕНТ ОТПРАВЛЯЕТ
                    timer.clear();
                    if(seq % packet.pckt_per_s == 0) print("%d seconds left.", packet.duration - seq / packet.pckt_per_s);
                    if(seq <= packet.amount) {
                        I2 status  = packetizer_tx->send(seq);
                        if(status > 0) seq++;
                    }
                }
                else if(queue == packetizer_sent){
                    //КЛИЕНТ ОТПРАВИЛ
                    U64 ts = packetizer_sent->utc_sent;
                    I4 sq = packetizer_sent->sequence;
                    if(!msmt[sq].up_ts) msmt[sq].up_ts = ts;
                    packetizer_sent->clear();
                }
            }
        }
    }
};


Core* new_Core(WaitSystem* waitSystem, Core::Setup &setup) {
    return new CoreObject(waitSystem, setup);
}
