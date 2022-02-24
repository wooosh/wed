#pragma once

#include "../Platform/VirtualMemory.hxx"
#include "../Util/Assert.hxx"
#include "ConstView.hxx"
#include <cstring>
#include <sys/mman.h>
#include <vector>

template <typename T> struct AppendBuffer {
  /* TODO: replace Chunk with ConstView<T> and/or write a split
   * function/template, can't use constview though because we need to write to
   * it*/
  struct Chunk {
    VirtualMemory::Handle vmem;

    T *addr;
    /* number of elements, not bytes */
    size_t len;
    size_t capacity;

    size_t remaining() { return capacity - len; }
    T *end() { return addr + len; }
  };
  std::vector<Chunk> chunks;

  /* allocate a new chunk and append it to chunks */
  // void PushNewChunk(size_t min_capacity);

  const size_t min_chunk_capacity = (16 * 1024 * 1024) / sizeof(T);

  ~AppendBuffer();

  template <typename ForwardIter>
  ConstView<T> CopyFrom(const ForwardIter begin, const ForwardIter end);
};

template <typename T> AppendBuffer<T>::~AppendBuffer() {
  for (Chunk chunk : chunks) {
    VirtualMemory::Unmap(chunk.vmem);
  }
}

/*
template <typename T> void AppendBuffer<T>::PushNewChunk(size_t min_capacity) {
  size_t capacity = std::max(min_capacity, min_chunk_capacity);

  void *ptr = mmap(nullptr, capacity * sizeof(T), PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assume(ptr != MAP_FAILED, strerror(errno));

  chunks.push_back({static_cast<T *>(ptr), 0, capacity});
}*/

template <typename T>
template <typename ForwardIter>
ConstView<T> AppendBuffer<T>::CopyFrom(const ForwardIter begin,
                                       const ForwardIter end) {
  // allocate new chunk if capacity < len
  size_t len = std::distance(begin, end);
  if (chunks.size() == 0 || chunks.back().remaining() < len) {
    Chunk new_chunk;
    new_chunk.len = 0;
    new_chunk.capacity = std::max(len, min_chunk_capacity);
    new_chunk.vmem =
        VirtualMemory::NewPrivateMemory(new_chunk.capacity * sizeof(T));
    new_chunk.addr = static_cast<T *>(VirtualMemory::Map(new_chunk.vmem, true));
    chunks.push_back(new_chunk);
  }

  Chunk &chunk = chunks.back();
  T *start = chunk.end();
  std::copy(begin, end, chunk.end());
  chunk.len += len;
  return {start, len};
}
