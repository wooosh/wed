#pragma once

#include "AppendBuffer.hxx"
#include <iterator>
#include <memory>
#include <vector>

/*
 EXTERNAL INTERFACE
  All external usage of the buffer should be handled by using iterators.

  TextBuffer iterators conform to the STL bidirectional iterator interface.

  Iterators are invalidated on any edit unless explicitly persisted using the
  PersistIterator function, which relies on the TextBuffer having access to the
  iterator through a shared pointer at all times.

  Invalid iterators will be automatically detected in when debug mode is
  enabled. If you want to test if an iterator is valid yourself, compare the
  epoch value of the iterator matches the epoch value of the TextBuffer

  Iterators can be created at a position with one of the following functions:
  * TextBuffer::iterator AtByteOffset(size_t byte_offset)
  * TextBuffer::iterator AtLineCol(size_t line, size_t column)

  Iterators are invalidated on any edit unless explicitly persisted using the
  PersistIterator method, which relies on the TextBuffer having access to the
  iterator through a shared pointer at all times.

  Modifications to the buffer can be made using one of the following two
  functions:
  * void InsertAt(TextBuffer::iterator position,
    const ForwardIterator begin, const ForwardIterator end)
  * void DeleteBetween(TextBuffer::iterator start, TextBuffer::iterator end)

DESIGN
  TextBuffer provides a efficient data structure for editing text

  The priorities are as follows:
  * Low, relatively constant write latency
  * Maximize read throughput
  * Minimize memory overhead
    * Copy on write behavior when sharing a TextBuffer with other threads or
      processes
  * Simplest implementation for desired performance
  * Easy undo implementation

  A slightly modified piece table fufills all of these criteria.

  Overall, a TextBuffer consists of the following components:
  * A vector of the current spans
    * Spans store a reference their contents, as well as pointers to the
      newline characters within them to speed up line based lookup
  * A custom allocator providing append only storage for the contents and
    newline locations offsets
    * The allocator uses a chunk system to ensure allocated data will never be
      relocated in memory, unlike malloc/realloc
    * A large chunk size is used by default to amortize the cost of appending
    * mmap() is used to ensure that the memory we do not touch is not backed
      by physical memory, so there is no cost to allocating large chunks
  * An "epoch" value indicating the buffer version, incremented on every change

PERFORMANCE SCALING
  Generally speaking, most operations will scale with the number of spans
  currently in the array.

  Modification:
  * insert
    * worst case O(n + m) time complexity, where n is the length of the string,
      and m is the number of spans
    * when multiple contiguous inserts are made, such as a user typing without
      navigating around the buffer, the time complexity is O(n), since no new
      spans have to be inserted
  * delete
    * worst case O(n) time complexity, where n is the number of spans
    * if a delete does not cross multiple spans, the complexity is O(1)

  Iterators:
  * index by line
    * always O(n + m) where n is the number of spans, and m is the number of
      lines in the span that the requested line resides in
  * index or advance by byte offset
    * always O(n) where n is the number of spans
  * increment iterator
    * always O(1)
  * compare iterator
    * always O(1)

CONCURRENT USAGE
  To be documented...

TODO
  * num_lines, num_bytes
  * implement IsEOF, ReadLine, ReadSpan
  * document IsEOF, ReadLine, ReadSpan
  * persistent iterators
  * implement operator-(iterator, iterator)
  * delete operation
  * document empty document state/EOF span
  * document iterator invariant where iterators must only have 1 valid state for
    a given position in the document
  * document iterator invariant that iterators must always be able to
    dereference a character unless they are at the end of the document
  * undo/redo
  * concurrency
*/

/* span-table based data structure for editing text */
struct TextBuffer {
  struct iterator;

  struct Span {
    ConstView<uint8_t> contents;
    ConstView<const uint8_t *> newline_ptrs;
  };

  /* allocators for span data */
  AppendBuffer<uint8_t> content_buffer;
  AppendBuffer<const uint8_t *> newline_ptr_buffer;

  std::vector<Span> spans;

  /* monotonically increasing buffer version incremented every edit */
  size_t epoch;

  size_t num_lines;
  size_t num_bytes;

  /* TODO: handle dead iterators? RAII remove them from array? */
  // std::vector<std::weak_ptr<iterator>> persisted_iterators;

  TextBuffer();
  TextBuffer(const TextBuffer &) = delete;
  TextBuffer &operator=(const TextBuffer &) = delete;

  void AssertValidIterator(iterator i);
  void NewEpoch();

  iterator AtByteOffset(size_t byte_offset);
  iterator AtLineCol(size_t line, size_t col);

  // std::shared_ptr<iterator> PersistIterator(iterator i);

  void DeleteBetween(iterator, iterator);

  /* TODO: document what happens to pos */
  std::vector<const uint8_t *> newline_offset_scratch;
  template <typename ForwardIter>
  void InsertAt(iterator pos, const ForwardIter begin, const ForwardIter end);
};

/* ForwardIterator into TextBuffer.
 * Invalidated  on edit unless explicitly persisted using
 * TextBuffer::PersistIterator
 * NOTE: lowercase to conform with existing iterator convention in libstdc++
 */
struct TextBuffer::iterator {
  using difference_type = std::ptrdiff_t;
  using value_type = uint8_t;
  using pointer = uint8_t *;
  using reference = uint8_t &;
  using iterator_category = std::bidirectional_iterator_tag;

  TextBuffer *parent;
  /* buffer epoch that the iterator is valid for */
  size_t epoch;

  size_t span_idx;
  /* number of bytes into the current span */
  size_t byte_offset;

  /* Returns true if the iterator is at the end of the file */
  bool IsEOF() const;

  /* Returns the remainder of the current span, and advances to the next.
   * Returns the last valid span in the case of EOF */
  ConstView<uint8_t> AdvanceSpan();

  /* Writes the remainder of the current line into out, and sets the cursor
   * position to the beginning of the next line.
   * Returns true on success, false on failure */
  template <typename OutputIterator> bool ReadLine(OutputIterator out);

  /* operator overloads for iterator */
  iterator &operator+=(size_t);
  iterator &operator++();
  iterator operator++(int);
  friend iterator operator+(iterator, size_t);
  friend iterator operator+(size_t, iterator);

  iterator &operator-=(size_t);
  iterator &operator--();
  iterator operator--(int);
  friend iterator operator-(iterator, size_t);
  // friend ssize_t operator-(iterator, iterator);

  friend bool operator<(const iterator &, const iterator &);
  friend bool operator>(const iterator &, const iterator &);
  friend bool operator<=(const iterator &, const iterator &);
  friend bool operator>=(const iterator &, const iterator &);

  const uint8_t &operator*() const;
};

#include "InsertAt.ipp"
