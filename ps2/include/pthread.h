/* pthread.h for the PS2 port: one thread. Mutexes do nothing, a thread is
   never created (pthread_create fails: Xash3D's DNS resolver, net_ws.c,
   then resolves in place, and without a network fails at once). */
#ifndef PS2_PTHREAD_H
#define PS2_PTHREAD_H

#include <errno.h>

typedef int pthread_t;
typedef int pthread_mutex_t;
typedef int pthread_mutexattr_t;
typedef int pthread_attr_t;

#ifdef __cplusplus
extern "C" {
#endif
static inline int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a) { return 0; }
static inline int pthread_mutex_destroy(pthread_mutex_t *m) { return 0; }
static inline int pthread_mutex_lock(pthread_mutex_t *m) { return 0; }
static inline int pthread_mutex_unlock(pthread_mutex_t *m) { return 0; }
static inline int pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*f)(void *), void *arg) {
  return EAGAIN;
}
static inline int pthread_detach(pthread_t t) { return 0; }
static inline int pthread_join(pthread_t t, void **r) { return EINVAL; }
#ifdef __cplusplus
}
#endif

#endif
