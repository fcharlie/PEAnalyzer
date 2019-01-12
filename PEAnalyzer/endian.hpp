/////////
#ifndef PLANCK_ENDIAN_HPP
#define PLANCK_ENDIAN_HPP

#if defined(__linux__) || defined(__GNU__) || defined(__HAIKU__)
#include <endian.h>
#elif defined(_AIX)
#include <sys/machine.h>
#elif defined(__sun)
/* Solaris provides _BIG_ENDIAN/_LITTLE_ENDIAN selector in sys/types.h */
#include <sys/types.h>
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234
#if defined(_BIG_ENDIAN)
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif
#else
#if !defined(BYTE_ORDER) && !defined(_WIN32)
#include <machine/endian.h>
#endif
#endif

#ifndef PLANCK_GNUC_PREREQ
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define PLANCK_GNUC_PREREQ(maj, min, patch)                                    \
  ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__ >=          \
   ((maj) << 20) + ((min) << 10) + (patch))
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#define PLANCK_GNUC_PREREQ(maj, min, patch)                                    \
  ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) >= ((maj) << 20) + ((min) << 10))
#else
#define PLANCK_GNUC_PREREQ(maj, min, patch) 0
#endif
#endif
#include <cstdint>

namespace planck {
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN
#define IS_BIG_ENDIAN 1
constexpr bool IsBigEndianHost = true;
#else
#define IS_BIG_ENDIAN 0
constexpr bool IsBigEndianHost = false;
#endif

/// Dump todo
inline bool IsBigEndian() { return IsBigEndianHost; }

inline uint16_t bswap16(uint16_t value) {
#if defined(_MSC_VER) && !defined(_DEBUG)
  // The DLL version of the runtime lacks these functions (bug!?), but in a
  // release build they're replaced with BSWAP instructions anyway.
  return _byteswap_ushort(value);
#else
  uint16_t Hi = value << 8;
  uint16_t Lo = value >> 8;
  return Hi | Lo;
#endif
}

inline uint32_t bswap32(uint32_t value) {
#if defined(__llvm__) || (PLANCK_GNUC_PREREQ(4, 3, 0) && !defined(__ICC))
  return __builtin_bswap32(value);
#elif defined(_MSC_VER) && !defined(_DEBUG)
  return _byteswap_ulong(value);
#else
  uint32_t Byte0 = value & 0x000000FF;
  uint32_t Byte1 = value & 0x0000FF00;
  uint32_t Byte2 = value & 0x00FF0000;
  uint32_t Byte3 = value & 0xFF000000;
  return (Byte0 << 24) | (Byte1 << 8) | (Byte2 >> 8) | (Byte3 >> 24);
#endif
}

/// SwapByteOrder_64 - This function returns a byte-swapped representation of
/// the 64-bit argument.
inline uint64_t bswap64(uint64_t value) {
#if defined(__llvm__) || (PLANCK_GNUC_PREREQ(4, 3, 0) && !defined(__ICC))
  return __builtin_bswap64(value);
#elif defined(_MSC_VER) && !defined(_DEBUG)
  return _byteswap_uint64(value);
#else
  uint64_t Hi = bswap32(uint32_t(value));
  uint32_t Lo = bswap32(uint32_t(value >> 32));
  return (Hi << 32) | Lo;
#endif
}
inline uint8_t bswap(uint8_t c) { return c; }
inline int8_t bswap(int8_t c) { return c; }
inline uint16_t bswap(uint16_t value) { return bswap16(value); }
inline int16_t bswap(int16_t value) {
  return static_cast<int16_t>(bswap16(value));
}
inline uint32_t bswap(uint32_t value) { return bswap32(value); }
inline int32_t bswap(int32_t value) {
  return static_cast<int32_t>(bswap32(value));
}

#if __LONG_MAX__ == __INT_MAX__
inline unsigned long bswap(unsigned long C) { return bswap32(C); }
inline signed long bswap(signed long C) { return bswap32(C); }
#elif __LONG_MAX__ == __LONG_LONG_MAX__
inline unsigned long bswap(unsigned long C) { return bswap64(C); }
inline signed long bswap(signed long C) { return bswap64(C); }
#else
#error "Unknown long size!"
#endif
inline unsigned long long bswap(unsigned long long C) { return bswap64(C); }
inline signed long long bswap(signed long long C) { return bswap64(C); }

template <typename Integer> Integer resolvele(Integer i) {
#if IS_BIG_ENDIAN
  return bswap(i);
#else
  return i;
#endif
}

template <typename Integer> Integer resolvebe(Integer i) {
#if IS_BIG_ENDIAN
  return i;
#else
  return bswap(i);
#endif
}

template <typename Integer> Integer readle(const void *p) {
  auto i = *reinterpret_cast<const Integer *>(p);
#if IS_BIG_ENDIAN
  return bswap(i);
#else
  return i;
#endif
}

template <typename Integer> Integer readbe(const void *p) {
  auto i = *reinterpret_cast<const Integer *>(p);
#if IS_BIG_ENDIAN
  return i;
#else
  return bswap(i);
#endif
}
} // namespace planck

#endif
