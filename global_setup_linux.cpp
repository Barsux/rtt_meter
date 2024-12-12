#include "global_setup_linux.h"

class Global_setupObject: public WaitSystem::Module, public Global_setup{
public:
    char * path;
    bool setted;
    class Set : public Queue_toSet {public:
        Global_setupObject &base;
        Set(Global_setupObject &base): base(base){}
        struct settings get_values(char * dir){
            return base.get_settings(dir);
        }
    } setup_set;

    class Save : public Queue_toSave {public:
        Global_setupObject &base;
        Save(Global_setupObject &base): base(base){}
        int save_values(struct settings config){
        }
    } setup_save;

    Global_setupObject(WaitSystem* waitSystem): WaitSystem::Module(waitSystem)
            , setted(false), path("cfg.json"), setup_set(*this), setup_save(*this)
    {
        module_debug = "SETUP";
        save = &setup_save;
        set = &setup_set;
        enable_wait(save);
    }

    struct settings get_settings(char * dir){
    }

    void save_values(struct settings cfg){
    }

    void evaluate(){
        if(!setted){
            setted = true;
        }
        while (WaitSystem::Queue* queue = enum_ready_queues()){
        }
    }
};
Global_setup* new_Global_setup(WaitSystem* waitSystem){
    return new Global_setupObject(waitSystem);
}
