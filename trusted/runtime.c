
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "minsfi.h"
#include "minsfi-inl.h"

/* Template data for the data segment. */
extern char __sfi_data_segment[];
extern uint32_t __sfi_data_segment_size;

/* Location of data segment: must match AllocateDataSegment. */
static const sb_ptr_t data_segment_dest = 0x10000;

/* Size of sandbox guard regions: must match SandboxMemoryAccesses. */
static const size_t sandbox_guard_size = 0x10000;

static sb_ptr_t next_alloc;

uint64_t __sfi_memory_base;

int sandbox_entry(int argc, sb_ptr_t argv);

static void init_sandbox() {
  assert(__sfi_memory_base == 0);

  size_t sandbox_size = ((size_t) 1 << 32) + sandbox_guard_size;
  void *alloc = mmap(NULL, sandbox_size, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
  assert(alloc != MAP_FAILED);
  __sfi_memory_base = (uintptr_t) alloc;

  copy_in(data_segment_dest, __sfi_data_segment, __sfi_data_segment_size);
}

/* Allocate memory in the sandbox's address space. */
static sb_ptr_t alloc_untrusted(sb_size_t size) {
  if (next_alloc == 0) {
    next_alloc = data_segment_dest + __sfi_data_segment_size;
  }
  sb_ptr_t addr = next_alloc;
  next_alloc += size;
  return addr;
}

int main(int argc, const char *argv[]) {
  init_system_io(argc, argv);
  init_sandbox();

  sb_ptr_t argv_copy = alloc_untrusted((argc + 1) * sizeof(sb_ptr_t));
  int i;
  for (i = 0; i < argc; i++) {
    const char *arg = argv[i];
    sb_size_t len = strlen(arg);
    sb_ptr_t arg_copy = alloc_untrusted(len + 1);
    copy_in(arg_copy, arg, len + 1);
    copy_in(argv_copy + i * sizeof(sb_ptr_t), &arg_copy, sizeof(arg_copy));
  }

  return sandbox_entry(argc, argv_copy);
}
