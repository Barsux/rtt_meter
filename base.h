#ifndef baseH
#define baseH


#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include <stdarg.h>
#include <new>
#ifdef __linux__
#include "sys/socket.h"
#include <sys/select.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <linux/if_packet.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include "inttypes.h"
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#else
#include "windows.h"
#include "mem.h"
#endif

#define BUNG return 1

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;
typedef unsigned int U4;
typedef unsigned short U2;
typedef int64_t I64;
typedef int32_t I32;
typedef int16_t I16;
typedef int8_t I8;
typedef int I4;
typedef short I2;
typedef U64 UTC;
typedef U8 MAC[6];
typedef U32 IP4;
typedef char * STR;
typedef unsigned char UC;

struct TIME_INFO {// ??? ???? ?????? ???? ???? int (????????, ??? scanf)
  U32 YYYY, MM, DD, w, hh, mm, ss, ns;
  TIME_INFO(): YYYY(), MM(), DD(), hh(), mm(),ss(), ns(), w() {}
};
void unpack_utc(TIME_INFO &ti, U64 utc);
void getmac(MAC &Mac, STR iface);
void getip4(IP4 &ip, STR iface);


template <typename T> void arr_add(T* &items, int &nItems, T item);
template <typename T> int arr_find(T* &items, int &nItems, T item);
template <typename T> void arr_cut_index(T* &items, int &nItems, int index);
template <typename T> void arr_add_to_set(T* &items, int &nItems, T item);
template <typename T> void arr_remove(T* &items, int &nItems, T item);
template <typename T> void arr_free(T* &items, int &nItems);

U64 nanotime();
void stdout_printf(char* fmt, ...);
void print(char* fmt, ...);

int str2int(int &dst, char * src);
int str2ip4(const char * dst, IP4 ip);
int ip42str(char * dst, IP4 ip);
int utc2str(char* dst, int cbDstMax, U64 utc);
int mac2str(char* dst, MAC mac);
bool str2mac(MAC &dst, char* src, int cbSrc=-1);

#ifdef __linux__
#define socket_NONBLOCK(af, type, protocol) socket(af, type | SOCK_NONBLOCK, protocol)
#else
__inline int socket_NONBLOCK(int af, int type, int protocol) {
  int fd = socket(af, type, protocol); DWORD dw = 1; ioctlsocket(fd, FIONBIO, &dw); return fd;
}
#endif

struct pckt{
    bool is_server;
    MAC srcMAC, dstMAC;
    IP4 srcIP, dstIP;
    int srcPORT, dstPORT;
    int size, pckt_per_s, duration, amount;
    pckt():is_server(false), srcPORT(5850), dstPORT(5850), size(1024), pckt_per_s(1), duration(1) {}
};


class WaitSystemCore;
class WaitSystem {public:
  class Queue;
  class Module;
  virtual void add_module(Module* module) = 0;
  virtual void remove_module(Module* module) = 0;
  virtual void enable_wait(Module* module, Queue* queue) = 0;
  virtual void disable_wait(Module* module, Queue* queue) = 0;
  virtual void start_timer(Queue* queue, U64 nsec_interval) = 0;
  virtual void stop_timer(Queue* queue) = 0;
  virtual void run() = 0;

  class Module {
    friend WaitSystemCore;
    Queue** readyQueues; int nReadyQueues;
  public:
    enum Flags {
      evaluate_once_needed = 0x01,
      evaluate_every_cycle = 0x02,
    } flags;
    WaitSystem* waitSystem;
    char* module_debug;
    Module(WaitSystem* waitSystem): waitSystem(waitSystem), readyQueues(), nReadyQueues()
      , flags(Flags()), module_debug()
    {
      waitSystem->add_module(this);
    }
    virtual ~Module() {
      waitSystem->remove_module(this);
    }
    virtual void evaluate() = 0;
    Queue* enum_ready_queues();
    void print(char* fmt, ...);
    void enable_wait(Queue* queue) {waitSystem->enable_wait(this, queue);}
    void disable_wait(Queue* queue) {waitSystem->disable_wait(this, queue);}
  };
  class Queue {
    Module** listeners; int nListeners;
    U64 nsec_timerNext, nsec_timerInterval;
  public:
    friend WaitSystemCore;
    int nAvail;
    Queue(): nAvail(), listeners(), nListeners(), nsec_timerNext(), nsec_timerInterval() {}
    bool isReady() {return nAvail;}
    void setReady(int nAvail=1) {this->nAvail = nAvail;}
    void clear() {nAvail = 0;}
  };
};

WaitSystem* new_WaitSystem();
#endif
