#ifndef l2_transport_linuxH
#define l2_transport_linuxH

#include "l2_transport.h"

L2Transport* new_L2Transport(WaitSystem* waitSystem, L2Transport::Setup &setup);
#endif
