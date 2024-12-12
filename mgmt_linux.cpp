#include "mgmt_linux.h"

class MgmtObject: public WaitSystem::Module, public Mgmt {
    Mgmt::Setup &setup;
public:

    Global_setup::Queue_toSet*       setup_set;
    Global_setup::Queue_toSave*     setup_save;

    bool converted;
    class Job: public Queue_job {public:
        MgmtObject &base;
        Job(MgmtObject &base): base(base){}
        struct pckt convert(int argc, char **argv) {
            return base.convert(argc,  argv);
        }
    } mgmt_job;

    class Report: public Queue_report {public:
        MgmtObject &base;
        Report(MgmtObject &base): base(base){}
        void report(double avg_out, double min_out, double max_out, float percent, int packet_loss){
            return base.report_void(avg_out, min_out, max_out, percent, packet_loss);
        }
    } mgmt_report;
    MgmtObject(WaitSystem* waitSystem, Mgmt::Setup &setup): WaitSystem::Module(waitSystem)
            , setup(setup), converted(false), mgmt_job(*this), mgmt_report(*this)
    {
        module_debug = "Managment";
        job = &mgmt_job;
        report = &mgmt_report; enable_wait(report);
    }

    void attach_Global_setup(Global_setup::Queue_toSave* save, Global_setup::Queue_toSet* set){
        setup_set = set; setup_save = save;
        disable_wait(setup_set); disable_wait(setup_save);
        enable_wait(setup_set);
    }

    void report_void(double avg_out, double min_out, double max_out, float percent, int packet_loss){
        printf( "\n\n/====================================\n"
               "|Average RTT             : %0.4fms\n"
               "|Minimum RTT             : %0.4fms\n"
               "|Maximum RTT             : %0.4fms\n"
               "|Total loss packets      : %d\n"
               "|Percent of loss packets : %0.4f%%\n"
               "\\====================================\n",
               avg_out,
               min_out,
               max_out,
               packet_loss,
               percent);
        exit(EXIT_SUCCESS);
    }
    struct pckt convert(int argc, char **argv){
        struct pckt data;
        if(argc == 6){
            getip4(data.srcIP, setup.iface);
            char ip[15]; memset(ip, 0, 15);
            ip42str(ip, data.srcIP); print("%s", ip);
            getmac(data.srcMAC, setup.iface);
            str2mac(data.dstMAC, argv[1]);
            data.dstIP = inet_addr(argv[2]);
            str2int(data.size, argv[3]);
            str2int(data.pckt_per_s, argv[4]);
            str2int(data.duration, argv[5]);
            data.amount = data.duration * data.pckt_per_s;
        }
        else if(argc == 2){
            str2int(data.duration, argv[1]);
            data.is_server = true;
        }
        return data;
    }
    void evaluate(){
        if(!converted) {
            job->packet = convert(setup.argc, setup.argv);
            job->setReady();
        }
    }


};

Mgmt* new_Mgmt(WaitSystem* waitSystem, Mgmt::Setup &setup){
    return new MgmtObject(waitSystem, setup);
}
