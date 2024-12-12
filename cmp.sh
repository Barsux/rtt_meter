#/bin/bash
g++ -Wall -Wfatal-errors -ffunction-sections -fdata-sections -Wl,--gc-sections -Wwrite-strings -Wreorder -fpermissive -fno-rtti -fno-exceptions -g -o bin base.cpp core.cpp l2_transport_linux.cpp main.cpp mgmt_linux.cpp global_setup_linux.cpp packetizer_linux.cpp && echo sucscess
