#ifndef _BYTESWAP_H
#define _BYTESWAP_H

//#warning "byteswap.h is an unportable GNU extension! Don't use!"
// I presume that would be copyright "Joshua Wright
// <jwright-iGNaCUDxsatBDgjK7y7TUQ@xxxxxxxxxxxxxxxx>", it's GPL code (COPYING 
// incl. in tarball).

static inline unsigned short __bswap_16(unsigned short x) {
  return (x>>8) | (x<<8);
}

static inline unsigned int __bswap_32(unsigned int x) {
  return (__bswap_16(x&0xffff)<<16) | (__bswap_16(x>>16));
}

static inline unsigned long long __bswap_64(unsigned long long x) {
  return (((unsigned long long)__bswap_32(x&0xffffffffull))<<32) |
  (__bswap_32(x>>32));
}

#endif
