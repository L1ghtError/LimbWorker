#ifndef _BITHACKS_H_
#define _BITHACKS_H_
#include <chrono>

/**
 * @brief Computes nearest greater equal power of 2 comparing to v.
 * @param v value that need to be rounded
 * @return nearest greater equal power of 2.
 */
uint32_t nextPowerOfTwo(uint32_t v) {
  if (v <= 2)
    return 2;
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}
#endif // _BITHACKS_H_