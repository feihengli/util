#ifndef _TYPEDEF_H
#define _TYPEDEF_H

#include <stdint.h>

#ifndef FOR_64BIT_SYSTEM
typedef uint32_t ch_context_t;
typedef uint32_t md_context_t;
typedef uint32_t nl_context_t;
#else
typedef uint64_t ch_context_t;
typedef uint64_t md_context_t;
typedef uint64_t nl_context_t;
#endif

#endif /* _TYPEDEF_H */


