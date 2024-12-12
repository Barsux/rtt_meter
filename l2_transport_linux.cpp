#include "l2_transport_linux.h"
#ifdef __linux__
#include <linux/net_tstamp.h>
#include <linux/ethtool.h>
#else
#include "fake_linux.h"
#endif

class L2Transport_Linux: public WaitSystem::Module, public L2Transport {
  L2Transport::Setup &setup;
public:
  int fd, sequence;
  class Rx: public Queue_rx {public:
    L2Transport_Linux &base;
    Rx(L2Transport_Linux &base): base(base) {
    }
      int recv(void * dst, U64 &utc_rx , int maxsize) {
      return base.recv(dst, utc_rx, maxsize);
    }
  } queue_rx;
  class Tx: public Queue_tx {public:
    L2Transport_Linux &base;
    Tx(L2Transport_Linux &base): base(base) {
    }
    int send(U8 * buffer, int seq, int size) {
      return base.send(buffer, seq, size);
    }
  } queue_tx;
  Queue_sent queue_sent;
  //WaitSystem::Queue txPackets;
  L2Transport_Linux(WaitSystem* waitSystem, L2Transport::Setup &setup): WaitSystem::Module(waitSystem)
    , setup(setup), fd(), queue_rx(*this), queue_tx(*this)
  {
    module_debug = "ETH";
    rx = &queue_rx; sent = &queue_sent;
    tx = &queue_tx; enable_wait(tx);
    flags |= evaluate_every_cycle;
  }

  void try_open() {
    fd = socket_NONBLOCK(PF_PACKET, SOCK_RAW, 0);
    if (fd<0) {print("error create PF_PACKET socket"); fd = 0; return;}
    ifreq ifr = {}; strcpy(ifr.ifr_name, setup.physicalId); int r; int i;
    r = ioctl(fd, SIOCGIFINDEX, &ifr);
    if (r<0) {print("SIOCGIFINDEX errno=%i", errno); close(fd); fd = 0; return;}
    sockaddr_ll saLL = {}; saLL.sll_family = AF_PACKET; saLL.sll_ifindex = ifr.ifr_ifindex;
    saLL.sll_protocol = htons(ETH_P_ALL);
    r = bind(fd, (sockaddr*)&saLL, sizeof(saLL));
    if (r<0) {print("bind errno=%i", errno); close(fd); fd = 0; return;}

    ifreq ifopts = {}; strncpy(ifopts.ifr_name, setup.physicalId, sizeof(ifopts.ifr_name));
    ioctl(fd, SIOCGIFFLAGS, &ifopts); ifopts.ifr_flags|=IFF_PROMISC; ioctl(fd, SIOCSIFFLAGS, &ifopts);

    i = 1; r = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, (const char*)&i, sizeof(i));
    if (r<0) {print("set SOF_TIMESTAMP errno=%i", errno); close(fd); fd = 0; return;}

    i = SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_TX_SOFTWARE;
    1; r = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, (const char*)&i, sizeof(i));
    if (r<0) {print("set SO_TIMESTAMPING errno=%i", errno); close(fd); fd = 0; return;}

    i = 1; r = setsockopt(fd, SOL_SOCKET, SO_SELECT_ERR_QUEUE, (const char*)&i, sizeof(i));
    if (r<0) {print("set SO_SELECT_ERR_QUEUE errno=%i", errno); close(fd); fd = 0; return;}

    i = 1; r = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPNS, (const char*)&i, sizeof(i));
    if (r<0) {print("set SO_TIMESTAMPNS errno=%i", errno); close(fd); fd = 0; return;}
  }

  int recv(void * dst, U64 &utc_rx , int maxsize) {
      iovec iovec; iovec.iov_base = dst; iovec.iov_len = maxsize;
      msghdr msg; msg.msg_iov = &iovec; msg.msg_iovlen = 1;
      sockaddr_ll sa_ll; msg.msg_name = &sa_ll; msg.msg_namelen = sizeof(sa_ll);
      U8 t[256]; msg.msg_control = t; msg.msg_controllen = sizeof(t); msg.msg_flags = 0;
      int r = recvmsg(fd, &msg, 0); if (r<=0) {
          //perror("fuck");
          return -1;
      }
      cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); utc_rx = 0;
      while (!utc_rx && cmsg) {
          if (cmsg->cmsg_level==SOL_SOCKET && cmsg->cmsg_type==SCM_TIMESTAMPNS) {
              timespec* ts = (timespec*)CMSG_DATA(cmsg);
              utc_rx = (U64)(ts->tv_sec)*1000000000ULL + ts->tv_nsec; break;
          }
          cmsg = CMSG_NXTHDR(&msg, cmsg);
      }
      return r;
  }

  int send(U8 * buffer, int seq, int size) {
    iovec iovec; iovec.iov_base = buffer; iovec.iov_len = size;
    msghdr msg = {}; msg.msg_iov = &iovec; msg.msg_iovlen = 1;
    sequence = seq;
    int cb = sendmsg(fd, &msg, MSG_DONTWAIT);
    return cb>=0? cb: -1;
  }


  void evaluate() { 
    if (!fd) try_open();
    if (!fd) {rx->clear(); return;}
    fd_set fds_rx ; FD_ZERO(&fds_rx ); FD_SET(fd, &fds_rx );
    fd_set fds_tx ; FD_ZERO(&fds_tx ); FD_SET(fd, &fds_tx );
    fd_set fds_err; FD_ZERO(&fds_err); FD_SET(fd, &fds_err);
    timeval tv = {};
    int r = select(fd+1, &fds_rx, &fds_tx, &fds_err, &tv);
    if (FD_ISSET(fd, &fds_rx)) rx->setReady();
    if (FD_ISSET(fd, &fds_tx))tx->setReady();
    if (FD_ISSET(fd, &fds_err)) {
      msghdr msg = {};
      U8 t[256]; msg.msg_control = t; msg.msg_controllen = sizeof(t);
      int r = recvmsg(fd, &msg, MSG_ERRQUEUE);
      cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); sent->utc_sent = 0;
      while (!r && !sent->utc_sent && cmsg) {
        if (cmsg->cmsg_level==SOL_SOCKET && cmsg->cmsg_type==SCM_TIMESTAMPNS) {
          timespec* ts = (timespec*)CMSG_DATA(cmsg);
          sent->utc_sent = (U64)(ts->tv_sec)*1000000000ULL + ts->tv_nsec;
          sent->sequence = sequence;
          sent->setReady(); break;
        }
        cmsg = CMSG_NXTHDR(&msg, cmsg);
      }
    }
  }
};

L2Transport* new_L2Transport(WaitSystem* waitSystem, L2Transport::Setup &setup) {
  return new L2Transport_Linux(waitSystem, setup);
}


