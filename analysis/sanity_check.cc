#include <iostream>
#include <stack>
#include <cstdio>
#include <cstdlib>
#include "../common.h"

using namespace std;

#define BUF_LEN (1024)

bool validate(char *path) {
  size_t count = 0;
  MEMREF *buf = new MEMREF[BUF_LEN];
  FILE *fp = fopen(path, "rb");
  size_t nelm = 0;
  stack<intptr_t> call_stack;
  
  while ((nelm = fread(buf, sizeof(MEMREF), BUF_LEN, fp)) > 0) {
    for (size_t i = 0; i < nelm; ++i) {
      const MEMREF &mr = buf[i];
      if (mr.type == TRACE_FUNC_CALL) {
        call_stack.push(mr.addr);
        cerr << "Call to " << mr.addr << endl;
      } else if (mr.type == TRACE_FUNC_RET) {
        // check the top of the stack matches the ret entry
        if (call_stack.size() == 0) {
          cerr << "ERROR! No call for return ("
               << mr.addr << ")" << endl;
          cerr << "Number of processed trace entries: " << count << endl;
          return false;
        }
        intptr_t call_addr = call_stack.top();
        if (call_addr != mr.addr) {
          cerr << "ERROR! Call (" << call_addr
               << ") and return (" << mr.addr << ") do not match."
               << endl;
          cerr << "Number of processed trace entries: " << count << endl;
          return false;
        }
        call_stack.pop();
        cerr << "Return from " << mr.addr << endl;
      } else {
        // No check for memory reads and writes
      }
      ++count;      
    }
  }
  
  if (call_stack.size() != 0) {
    cerr << "ERROR! Missing return for call: ";
    while (call_stack.size() > 0) {
      cerr << call_stack.top() << " ";
      call_stack.pop();
    }
    cerr << endl;
    cerr << "Number of processed trace entries: " << count << endl;
    return false;
  }
  cerr << "Number of processed trace entries: " << count << endl;            
  return true;
}

int main(int argc, char *argv[]) {
  char *file_path = argv[1];
  if (!validate(file_path)) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

