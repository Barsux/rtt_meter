#include "base.h"

template <typename T> void arr_add(T* &items, int &nItems, T item) {
  items = (T*)realloc(items, sizeof(T*)*(nItems+1));
  items[nItems] = item; nItems++;
}
template <typename T> int arr_find(T* &items, int &nItems, T item) {
  for (int i=0; i<nItems; i++) if (items[i]==item) return i;
  return -1;
}
template <typename T> void arr_add_to_set(T* &items, int &nItems, T item) {
  if (arr_find(items, nItems, item)>=0) return;
  arr_add(items, nItems, item);
}
template <typename T> void arr_cut_index(T* &items, int &nItems, int i) {
  if (i<0 || i>=nItems) return;
  nItems--; memcpy(&items[i], &items[i+1], sizeof(T)*(nItems-i));
}
template <typename T> void arr_remove(T* &items, int &nItems, T item) {
  int i = arr_find(items, nItems, item); if (i<0) return;
  arr_cut_index(items, nItems, i); if (!nItems) items = NULL;
}
template <typename T> void arr_free(T* &items, int &nItems) {
  free(items); items = NULL; nItems = 0;
}

void unpack_utc(TIME_INFO &dst, U64 utc) {
  #ifdef __linux__
  struct tm tm = {}; dst.ns = utc%1000000000ULL; time_t t = utc/1000000000ULL; gmtime_r(&t, &tm);
  dst.YYYY = 1900+tm.tm_year; dst.MM = 1+tm.tm_mon; dst.DD = tm.tm_mday;
  dst.hh = tm.tm_hour; dst.mm = tm.tm_min; dst.ss = tm.tm_sec; dst.w = tm.tm_wday;
  #else
  #endif
}
int utc2str(char* dst, int cbDstMax, U64 utc) {
  TIME_INFO ti; unpack_utc(ti, utc);
  return snprintf(dst, cbDstMax, "%04u%02u%02u-%02u:%02u:%02u.%06u"
    , ti.YYYY, ti.MM, ti.DD, ti.hh, ti.mm, ti.ss, ti.ns/1000
  );
}

int str2ip4 (const char * dst, IP4 ip)
{
    unsigned int byte0, byte1, byte2, byte3;
    char fakeString[2];
    if (sscanf(dst, "%u.%u.%u.%u%1s", &byte3, &byte2, &byte1, &byte0, fakeString) == 4)
    {
        if ((byte3 < 256) && (byte2 < 256) && (byte1 < 256) && (byte0 < 256))
        {
            ip  = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + byte0;
            return 1;
        }
    }return 0;
}

int mac2str(char* dst, MAC mac) {
  return snprintf(dst, 19, "%02x:%02x:%02x:%02x:%02x:%02x"
    , (U32)mac[0], (U32)mac[1], (U32)mac[2], (U32)mac[3], (U32)mac[4], (U32)mac[5]
  );
}

int str2int(int &dst, char * src){
    return sscanf(src, "%i", &dst);
}

void getmac(MAC &mac, STR iface)
{
    struct ifreq s;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    strcpy(s.ifr_name, iface);
    if (ioctl(fd, SIOCGIFHWADDR, &s) == 0) {
        for (int i = 0; i < 6; i++)
            mac[i] = s.ifr_addr.sa_data[i];
    }
}
void getip4(IP4 &ip, STR iface){
    struct ifreq ifr;
    strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
    int fd=socket(AF_INET,SOCK_DGRAM,0);
    if (fd==-1) {
        return;
    }
    if (ioctl(fd,SIOCGIFADDR,&ifr)==-1) {
        int temp_errno=errno;
        close(fd);
        return;
    }
    close(fd);
    struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
    ip = inet_addr(inet_ntoa(ipaddr->sin_addr));
}

int ip42str(char * dst, IP4 ip){
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_addr.s_addr = ip;
    strncpy(dst, inet_ntoa(sa.sin_addr), 15);
    return 1;
}

bool str2mac(MAC &dst, char* src, int cbSrc) {
  memset(dst, 0, sizeof(dst)); if (cbSrc<0) cbSrc = strlen(src);
  if (cbSrc!=12 && cbSrc!=17) return false; U8* u = (U8*)dst; U8* uOver = u+6;
  char* srcOver = src+cbSrc; int n = 0; int nTotal = 0; while (src<srcOver && nTotal<12) {
    char c = *src++;
    if (c>='0' && c<='9') {*u = (*u<<4)+c-'0'   ; n++; nTotal++;} else
    if (c>='a' && c<='f') {*u = (*u<<4)+c-'a'+10; n++; nTotal++;} else
    if (c>='A' && c<='F') {*u = (*u<<4)+c-'A'+10; n++; nTotal++;} else
    if (c=='.' || c==':' || c=='-') {
      if (u>=uOver) break; n = 0;
    } else break;
    if (n>=2) {n = 0; u++;}
  }
  return (nTotal==12 && src==srcOver);
}

#ifdef __linux__
void stdout_write(void* src, int cbSrc) {
  write(1, src, cbSrc);
  fflush(stdout);
}
#else



void stdout_write(void* src, int cbSrc) {
  static HANDLE hStdOutput;
  if (!hStdOutput) {AllocConsole(); hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);}
  WriteFile(hStdOutput, src, cbSrc, NULL, NULL);
}
#endif
void stdout_printf(char* fmt, ...) {
  char buff[2048];
  va_list va; va_start(va, fmt);
  int n = vsnprintf(buff, (int)sizeof(buff)-2, fmt, va); buff[sizeof(buff)-1]=0;
  va_end(va);
  stdout_write(buff, n);
}
void print(char* fmt, ...) {
  va_list va; va_start(va, fmt); char buff[2048];
  int n = vsnprintf(buff, (int)sizeof(buff)-2, fmt, va);
  buff[sizeof(buff)-3]=0;
  if (n>=sizeof(buff)) n = sizeof(buff)-2;
  buff[n++] = '\r'; buff[n++] = '\n'; stdout_write(buff, n);
  va_end(va);
}

void WaitSystem::Module::print(char* fmt, ...) {
  if (!module_debug) return;
  char t[128]; utc2str(t, sizeof(t), nanotime());
  stdout_printf("(%s) [%s] ", t, module_debug);
  va_list va; va_start(va, fmt); char buff[2048];
  int n = vsnprintf(buff, (int)sizeof(buff)-2, fmt, va);
  buff[sizeof(buff)-3]=0;
  if (n>=sizeof(buff)) n = sizeof(buff)-2;
  buff[n++] = '\r'; buff[n++] = '\n'; stdout_write(buff, n);
  va_end(va);
}

U64 nanotime() {
  #ifdef __linux__
  timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
  return U64(ts.tv_sec)*1000000000ULL + U64(ts.tv_nsec);
  #else
  U64 time[2]; GetSystemTimeAsFileTime((FILETIME*)(&time[0]));
  return U64((time[0])*100ULL - 11644473600000000000ULL);
  #endif
}

class WaitSystemCore: public WaitSystem {
  Queue** queues; int nQueues;
  Module** modules; int nModules;
public:
  WaitSystemCore(): queues(), nQueues(), modules(), nModules() {}

  void enable_wait(Module* module, Queue* queue) {
    arr_add_to_set(queues, nQueues, queue);
    arr_add_to_set(queue->listeners, queue->nListeners, module);
  }
  void disable_wait(Module* module, Queue* queue) {
    if (!module || !queue) return;
    arr_remove(queue->listeners, queue->nListeners, module);
    if (!queue->nListeners) {
      arr_remove(queues, nQueues, queue); stop_timer(queue);
    }
    arr_remove(module->readyQueues, module->nReadyQueues, queue);
  }
  void add_module(Module* module) {
    arr_add_to_set(modules, nModules, module); module->flags |= Module::evaluate_once_needed;
  }
  void remove_module(Module* module) {
    arr_remove(modules, nModules, module);
  }

  void start_timer(Queue* queue, U64 nsec_interval) {
    queue->nsec_timerNext = nanotime()+nsec_interval; queue->nsec_timerInterval = nsec_interval;
  }
  void stop_timer(Queue* queue) {
    queue->nsec_timerNext = 0; queue->nsec_timerInterval = 0;
  }

  void evaluate() {
    U64 ns = nanotime();
    for (int i=0; i<nQueues; i++) {
      Queue* queue = queues[i];
      if (queue->nsec_timerInterval && (!queue->nsec_timerNext || queue->nsec_timerNext<=ns)) {
        queue->nsec_timerNext = ns+queue->nsec_timerInterval; queue->setReady();
      }
      if (!queue->isReady()) continue;
      for (int j=0; j<queue->nListeners; j++) {
        Module* module = queue->listeners[j];
        arr_add_to_set(module->readyQueues, module->nReadyQueues, queue);
      }
    }
    int i = 0; while (i<nModules) {
      Module* module = modules[i];
      if (
        module->nReadyQueues ||
        (module->flags & Module::evaluate_once_needed) ||
        (module->flags & Module::evaluate_every_cycle)
      )
      {
        module->flags &= ~Module::evaluate_once_needed;
        module->evaluate();
      }
      i++;
    }
  }

  void run() {
    while (true) evaluate();
  }
};

WaitSystem::Queue* WaitSystem::Module::enum_ready_queues() {
  if (!nReadyQueues) return NULL;
  Queue* queue = readyQueues[0]; arr_cut_index(readyQueues, nReadyQueues, 0);
  return queue;
}

WaitSystem* new_WaitSystem() {
  return new WaitSystemCore;
}
