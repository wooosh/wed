#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

namespace VirtualMemory {
struct Handle {
  size_t size;

  /* id used to identify shared memory between processes
   * TODO: use DuplicateHandle on  */
  std::optional<std::string_view> id;

  void *mapped_address;

#ifdef _WIN32
  HANDLE handle;
#else
  int file_descriptor;
#endif
};

/* create a process local shared memory mapping */
Handle NewPrivateMemory(size_t size);

Handle NewSharedMemory(size_t size);
bool RemoveSharedMemory(Handle);
/* TODO: immediately close once mapped? */
Handle OpenSharedMemory(std::string_view id);

/* map the memory handle into the current processes address space, and return
 * the pointer */
void *Map(Handle &, bool writable);
void Unmap(Handle &);
} // namespace VirtualMemory