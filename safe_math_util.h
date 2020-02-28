#ifndef _SAFE_MATH_UTIL_H_
#define _SAFE_MATH_UTIL_H_

// TODO: We may want to add checks for GCC/Clang version. We also want
// to add wrappers for other checked math functions.
#define __checked_uadd(a, b, out)   (__builtin_uadd_overflow(a, b, out))
#define __checked_uaddl(a, b, out)  (__builtin_uaddl_overflow(a, b, out))
#define __checked_uaddll(a, b, out) (__builtin_uaddll_overflow(a, b, out))

#if __riscv_xlen == 64
  #define checked_add_uintptr(a, b, out) (__checked_uaddl(a, b, out))
#else
  #define checked_add_uintptr(a, b, out) (__checked_uadd(a, b, out))
#endif

#endif /* _SAFE_MATH_UTIL_H_ */
