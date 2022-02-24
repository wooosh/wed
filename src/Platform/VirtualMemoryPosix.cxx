#include "../Util/Assert.hxx"
#include "VirtualMemory.hxx"
#include <cerrno>
#include <cstring>
#include <optional>
#include <sys/mman.h>
#include <unistd.h>

namespace VirtualMemory {

Handle NewPrivateMemory(size_t size) {
  return {size, std::nullopt, nullptr, -1};
}

/* shmem is roughly
 * handle.id = generate_uuid() // argv[0]-pid-uuid
 * handle.file_descriptor = shmem_open(handle.id.c_str(), O_RDWR | O_EXCL, )
 * int success = ftruncate(handle.file_descriptor, size);
 * */

/* map the memory handle into the current processes address space, and return
 * the pointer */
void *Map(Handle &handle, bool writable) {
  assume(handle.mapped_address == nullptr,
         "virtual memory handles cannot be mapped more than once");

  int flags = PROT_READ;
  if (writable)
    flags |= PROT_WRITE;

  handle.mapped_address =
      mmap(nullptr, handle.size, flags, MAP_PRIVATE | MAP_ANONYMOUS,
           handle.file_descriptor, 0);

  assume(handle.mapped_address != NULL, strerror(errno));

  return handle.mapped_address;
}

void Unmap(Handle &handle) {
  if (handle.mapped_address != nullptr) {
    int success = munmap(handle.mapped_address, handle.size);
    assume(success == 0, strerror(errno));
    handle.mapped_address = nullptr;
  }
}
} // namespace VirtualMemory