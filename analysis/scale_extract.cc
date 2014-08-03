#include <iostream>
#include <stack>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "../common.h"

using namespace std;

#define BUF_LEN (1024)

bool extract(char *input_path, char *output_path) {
  size_t count = 0;
  MEMREF *buf = new MEMREF[BUF_LEN];
  FILE *fp = fopen(input_path, "rb");
  FILE *out = fopen(output_path, "wb");  
  size_t nelm = 0;
  stack<intptr_t> call_stack;
  bool in_extraction = false;

    
  while ((nelm = fread(buf, sizeof(MEMREF), BUF_LEN, fp)) > 0) {
    for (size_t i = 0; i < nelm; ++i) {
      const MEMREF &mr = buf[i];
      if (mr.type == TRACE_FUNC_CALL) {
        call_stack.push(mr.addr);
        if (count > 0) in_extraction = true;
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
        if (in_extraction) {
          // Done
          fclose(out);
          cout << "Extraction success." << endl;
          return true;
        }
      } else {
        if (in_extraction) {
          assert(fwrite(&mr, sizeof(MEMREF), 1, out) == 1);
        }
      }
      ++count;      
    }
  }

  // SHOULD NOT REACH HERE
  cerr << "ERROR! No terminating return found." << endl;
  return false;
}

int main(int argc, char *argv[]) {
  char *input_path = argv[1];
  char *output_path = argv[2];  
  if (!extract(input_path, output_path)) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

