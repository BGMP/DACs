/* compat.h - Compatibility fixes for macOS */
#ifndef COMPAT_H
#define COMPAT_H

/* Include system headers first */
#include <sys/types.h>

/* Fix uint conflict */
#ifdef __APPLE__
/* On macOS, uint is already defined in sys/types.h */
#ifndef ushort
typedef unsigned short ushort;
#endif

#ifndef ulong
typedef unsigned long ulong;
#endif
#else
/* For other platforms */
  #ifndef uint
    typedef unsigned int uint;
  #endif

  #ifndef ushort
    typedef unsigned short ushort;
  #endif

  #ifndef ulong
    typedef unsigned long ulong;
  #endif
#endif

/* Fix MIN/MAX macros */
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#endif /* COMPAT_H */
