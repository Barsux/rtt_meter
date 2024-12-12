#ifndef GLOBAL_SETUP_H
#define GLOBAL_SETUP_H

#include "base.h"
struct settings{
    bool have_settings;
    struct mgmt{
        bool busy;
        mgmt(): busy(false){}
    }mgmt;
    struct packager{
        bool busy;
        int port;
        char * interface_name;
        packager(): busy(false){}
    }packager;
    struct core{
        bool busy;
        core(): busy(false){}
    }core;
    settings(): have_settings(false){}
};


class Global_setup{public:
    virtual ~Global_setup() {}
    class Queue_toSet: public WaitSystem::Queue {public:
        struct settings config;
    }* set;
    class Queue_toSave: public WaitSystem::Queue {public:
        struct settings config;
    }* save;
};

#endif GLOBAL_SETUP_H
