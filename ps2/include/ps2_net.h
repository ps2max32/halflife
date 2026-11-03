/* ps2_net.h: what lib/networking.h takes from the BSD socket headers, for
   the PS2, which has no network here: the types and constants, and calls
   that fail (ENOSYS). Network play is not ported; the game runs without
   it as it does on a PC without a network. */
#ifndef PS2_NET_H
#define PS2_NET_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;
typedef uint16_t sa_family_t;
typedef int socklen_t;

struct in_addr {
  in_addr_t s_addr;
};
struct sockaddr {
  sa_family_t sa_family;
  char sa_data[14];
};
struct sockaddr_in {
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
  unsigned char sin_zero[8];
};
struct hostent {
  char *h_name;
  char **h_aliases;
  int h_addrtype;
  int h_length;
  char **h_addr_list;
};
#define h_addr h_addr_list[0]

#define AF_INET 2
#define PF_INET AF_INET
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define IPPROTO_IP 0
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define SOL_SOCKET 0xffff
#define SO_REUSEADDR 0x0004
#define SO_BROADCAST 0x0020
#define SO_SNDBUF 0x1001
#define SO_RCVBUF 0x1002
#define SO_ERROR 0x1007
#define SO_LINGER 0x0080
#define TCP_NODELAY 1
#define INADDR_ANY ((in_addr_t)0)
/* IPv6, as Xash3D's net_ws.c uses it */
#define AF_INET6 10
#define PF_INET6 AF_INET6
#define AF_UNSPEC 0
#define IPPROTO_IPV6 41
#define IPV6_V6ONLY 26
#define IPV6_MULTICAST_LOOP 19
#define IPV6_JOIN_GROUP 20
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IP_TOS 1
#define IP_MULTICAST_LOOP 34
#define SO_RCVTIMEO 0x1006
#define SO_SNDTIMEO 0x1005
#define in6addr_any ((struct in6_addr){{0}})
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
struct in6_addr {
  uint8_t s6_addr[16];
};
struct sockaddr_in6 {
  sa_family_t sin6_family;
  in_port_t sin6_port;
  uint32_t sin6_flowinfo;
  struct in6_addr sin6_addr;
  uint32_t sin6_scope_id;
};
struct sockaddr_storage {
  sa_family_t ss_family;
  char ss_pad[126];
};
struct ipv6_mreq {
  struct in6_addr ipv6mr_multiaddr;
  unsigned ipv6mr_interface;
};
struct addrinfo {
  int ai_flags, ai_family, ai_socktype, ai_protocol;
  socklen_t ai_addrlen;
  struct sockaddr *ai_addr;
  char *ai_canonname;
  struct addrinfo *ai_next;
};
#define AI_PASSIVE 1
#define AI_NUMERICHOST 4
#define EAI_FAIL 4
#define NI_MAXHOST 1025

#define INADDR_NONE ((in_addr_t)0xffffffff)
#define INADDR_BROADCAST ((in_addr_t)0xffffffff)
#define INADDR_LOOPBACK ((in_addr_t)0x7f000001)
#define FIONBIO 0x8004667e
#define FIONREAD 0x4004667f
#define MSG_PEEK 2
#define SHUT_RDWR 2
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif

#ifdef __cplusplus
extern "C" {
#endif
static inline int ps2_net_fail(void) {
  errno = ENOSYS;
  return -1;
}
static inline int socket(int d, int t, int p) { return ps2_net_fail(); }
static inline int bind(int s, const struct sockaddr *a, socklen_t l) { return ps2_net_fail(); }
static inline int connect(int s, const struct sockaddr *a, socklen_t l) { return ps2_net_fail(); }
static inline int listen(int s, int b) { return ps2_net_fail(); }
static inline int accept(int s, struct sockaddr *a, socklen_t *l) { return ps2_net_fail(); }
static inline ssize_t send(int s, const void *b, size_t n, int f) { return ps2_net_fail(); }
static inline ssize_t sendto(int s, const void *b, size_t n, int f, const struct sockaddr *a,
                             socklen_t l) {
  return ps2_net_fail();
}
static inline ssize_t recv(int s, void *b, size_t n, int f) { return ps2_net_fail(); }
static inline ssize_t recvfrom(int s, void *b, size_t n, int f, struct sockaddr *a, socklen_t *l) {
  return ps2_net_fail();
}
static inline int shutdown(int s, int h) { return ps2_net_fail(); }
static inline int setsockopt(int s, int lv, int o, const void *v, socklen_t l) { return ps2_net_fail(); }
static inline int getsockopt(int s, int lv, int o, void *v, socklen_t *l) { return ps2_net_fail(); }
static inline int getsockname(int s, struct sockaddr *a, socklen_t *l) { return ps2_net_fail(); }
static inline int ioctl(int fd, unsigned long r, ...) { return ps2_net_fail(); }
static inline struct hostent *gethostbyname(const char *n) { return 0; }
static inline int getaddrinfo(const char *n, const char *s, const struct addrinfo *h,
                              struct addrinfo **r) {
  return EAI_FAIL;
}
static inline void freeaddrinfo(struct addrinfo *r) {}
static inline const char *gai_strerror(int e) { return "no network"; }
static inline int inet_pton(int af, const char *src, void *dst) { return 0; }
static inline const char *inet_ntop(int af, const void *src, char *dst, socklen_t n) {
  errno = ENOSYS;
  return 0;
}
static inline in_addr_t inet_addr(const char *cp) { return INADDR_NONE; }
static inline char *inet_ntoa(struct in_addr in) {
  static char s[16];
  unsigned a = in.s_addr;
  char *p = s;
  for (int i = 0; i < 4; i++, a >>= 8) {
    unsigned b = a & 255;
    if (b >= 100)
      *p++ = '0' + b / 100;
    if (b >= 10)
      *p++ = '0' + b / 10 % 10;
    *p++ = '0' + b % 10;
    *p++ = i < 3 ? '.' : 0;
  }
  return s;
}
#ifdef __cplusplus
}
#endif

#endif
