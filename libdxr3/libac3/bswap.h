#ifndef __BSWAP_H__
#define __BSWAP_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#else

#include <inttypes.h>

#ifdef WORDS_BIGENDIAN
// FIXME these need to actually swap ;)
#define bswap_16(x) (x)
#define bswap_32(x) (x)
#define bswap_64(x) (x)
#else
// This is wrong, 'cannot take address of ...'
#define bswap_16(x) ((((uint8_t*)&x)[2] << 8) \
			| (((uint8_t*)&x)[3]))

// code from bits/byteswap.h (C) 1997, 1998 Free Software Foundation, Inc.
#define bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#define bswap_64(x) \
     (__extension__						\
      ({ union { __extension__ uint64_t __ll;                   \
                 uint32_t __l[2]; } __w, __r;                   \
         __w.__ll = (x);					\
         __r.__l[0] = bswap_32 (__w.__l[1]);			\
         __r.__l[1] = bswap_32 (__w.__l[0]);			\
         __r.__ll; }))
#endif

#endif

// be2me ... BigEndian to MachineEndian
// le2me ... LittleEndian to MachineEndian

#ifdef WORDS_BIGENDIAN
#define be2me_16(x) (x)
#define be2me_32(x) (x)
#define be2me_64(x) (x)
#define le2me_16(x) bswap_16(x)
#define le2me_32(x) bswap_32(x)
#define le2me_64(x) bswap_64(x)
#else
#define be2me_16(x) bswap_16(x)
#define be2me_32(x) bswap_32(x)
#define be2me_64(x) bswap_64(x)
#define le2me_16(x) (x)
#define le2me_32(x) (x)
#define le2me_64(x) (x)
#endif

#endif
