#/bin/bash
g++ -Wall -Wfatal-errors -ffunction-sections -fdata-sections -Wl,--gc-sections -Wwrite-strings -Wreorder -fpermissive -fno-rtti -fno-exceptions -g base.cpp core.cpp l2_transport_linux.cpp main.cpp mgmt_linux.cpp global_setup_linux.cpp packetizer_linux.cpp && sudo ./a.out CE:20:AF:AF:E7:ED 192.168.1.67 1024 10 10

