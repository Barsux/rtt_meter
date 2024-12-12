#ifndef PACKETIZER_H
#define PACKETIZER_H

#include "base.h"
#include "l2_transport_linux.h"
#include "global_setup_linux.h"

class Packetizer{public:
    virtual ~Packetizer() {}
    class Setup {public:
    };
    class Queue_prx: public WaitSystem::Queue {public:
        struct pckt packet;
        virtual int recv(int &seq, U64 &tstmp) = 0;
    }* rx;
    class Queue_ptx: public WaitSystem::Queue {public:
        virtual int send(int seq) = 0;
    }* tx;
    class Queue_psent: public WaitSystem::Queue {public:
        U64 utc_sent;
        int sequence;
    }* sent;
    virtual void attach_l2_transport(
            L2Transport::Queue_rx*   l2_transport_rx,
            L2Transport::Queue_tx*   l2_transport_tx,
            L2Transport::Queue_sent* sent
    ) = 0;

    virtual void attach_Global_setup(
            Global_setup::Queue_toSave* setup_save,
            Global_setup::Queue_toSet* setup_set
    ) = 0;
};




#endif PACKETIZER_H
