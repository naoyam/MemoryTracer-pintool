#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

enum {
  TRACE_READ,
  TRACE_WRITE,
  TRACE_FUNC_CALL,
  TRACE_FUNC_RET
};

struct MEMREF {
  intptr_t addr;
  uint32_t type;
  uint32_t size;
};

#endif /* COMMON_H_ */


