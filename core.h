#ifndef coreH
#define coreH

#include "base.h"
#include "l2_transport_linux.h"
#include "mgmt_linux.h"
#include "global_setup_linux.h"
#include "packetizer_linux.h"

struct measurement{
    I4 seq;
    U64 in_ts;
    U64 up_ts;
    measurement():seq(0), in_ts(0), up_ts(0){}
};

class Core {public:
  virtual ~Core() {}
  class Setup {public:
  };


  virtual void attach_mgmt(
    Mgmt::Queue_job* mgmt_job,
    Mgmt::Queue_report* mgmt_report
  ) = 0;

  virtual void attach_Global_setup(
    Global_setup::Queue_toSave* setup_save,
    Global_setup::Queue_toSet* setup_set
  ) = 0;

  virtual void attach_packetizer(
    Packetizer::Queue_prx* prx,
    Packetizer::Queue_ptx* ptx,
    Packetizer::Queue_psent* psent
  ) = 0;

};

Core* new_Core(WaitSystem* waitSystem, Core::Setup &setup);


#endif
