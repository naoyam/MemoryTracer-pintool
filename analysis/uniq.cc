#include <iostream>
#include <map>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <omp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../common.h"

using namespace std;

#define BUF_LEN (1024)

typedef map<intptr_t, size_t> addr_map;

bool uniq(char *path) {
  cerr << "MEMREF: " << sizeof(MEMREF) << endl;
  struct stat sb;
  assert(stat(path, &sb) == 0);
  off_t fs = sb.st_size;
  cerr << "File size: " << fs << endl;
  assert((fs % sizeof(MEMREF)) == 0);
  size_t total_nelm = fs / sizeof(MEMREF);
  cerr << "Total number of elements: " << total_nelm << endl;
  size_t num_chunks = total_nelm / BUF_LEN;
  if (total_nelm % BUF_LEN) ++num_chunks;
  cerr << "Number of chunks: " << num_chunks << endl;
  size_t count = 0;
  addr_map *rm  = NULL;
  addr_map *wm  = NULL;
  int num_maps = 0;
  
#pragma omp parallel reduction(+:count)
  {
#pragma omp single
    {
      cerr << "Number of threads: " << omp_get_num_threads() << endl;
      rm = new addr_map[omp_get_num_threads()];
      wm = new addr_map[omp_get_num_threads()];
      num_maps = omp_get_num_threads();
    }
    int tid = omp_get_thread_num();
    MEMREF *buf = new MEMREF[BUF_LEN];
    FILE *fp = fopen(path, "rb");
#pragma omp for schedule(static)
    for (size_t i = 0; i < num_chunks; ++i) {
      fseek(fp, i * BUF_LEN * sizeof(MEMREF), SEEK_SET);
      size_t nelm = fread(buf, sizeof(MEMREF), BUF_LEN, fp);
      for (size_t ni = 0; ni < nelm; ++ni) {
        const MEMREF &mr = buf[ni];
        if (mr.type == TRACE_FUNC_CALL) {
          cerr << "Call to " << mr.addr << endl;
        } else if (mr.type == TRACE_FUNC_RET) {
          cerr << "Retrun from " << mr.addr << endl;          
        } else if (mr.type == TRACE_READ) {
          pair<addr_map::iterator, bool> ret = rm[tid].insert(make_pair(mr.addr, 0));
          if (!ret.second) {
            ++ret.first->second;
          }
        }
      }
    }
  }
  cerr << "Number of elements processed: " << count << endl;

  for (int i = 1; i < num_maps; ++i) {
    addr_map::iterator it = rm[i].begin();
    addr_map::iterator it_end = rm[i].end();
    for (; it != it_end; ++it) {
      pair<addr_map::iterator, bool> ret = rm[0].insert(*it);
      if (!ret.second) {
        ret.first->second += it->second;
      }
    }
  }

  return true;  
}

int main(int argc, char *argv[]) {
  char *file_path = argv[1];
  if (!uniq(file_path)) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

