/* alloca.h of the port, in front of picolibc's (which declares a function:
   its config has no _HAVE_BUILTIN_ALLOCA): the compiler's alloca */
#ifndef PS2_ALLOCA_H
#define PS2_ALLOCA_H
#undef alloca
#define alloca(size) __builtin_alloca(size)
#endif
