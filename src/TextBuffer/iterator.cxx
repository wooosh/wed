#include "TextBuffer.hxx"
#include <cassert>

using iterator = TextBuffer::iterator;

void TextBuffer::AssertValidIterator(iterator i) {
  assert(i.parent == this);
  assert(i.epoch == epoch);
  assert(i.span_idx < spans.size());
  /* TODO: how to handle cursor at end of span? */
  assert(i.byte_offset <= spans[i.span_idx].contents.len);
}

/* constructors */

iterator TextBuffer::AtByteOffset(size_t byte_offset) {
  size_t bytes_traversed = 0;
  for (size_t i = 0; i < spans.size(); i++) {
    if (byte_offset <= bytes_traversed + spans[i].contents.len) {
      return {this, epoch, i, byte_offset - bytes_traversed};
    }

    bytes_traversed += spans[i].contents.len;
  }

  unreachable("attempted to create iterator with byte offset outside buffer");
}

iterator TextBuffer::AtLineCol(size_t line, size_t col) {
  size_t lines_traversed = 0;
  for (size_t i = 0; i < spans.size(); i++) {
    Span span = spans[i];
    if (line < lines_traversed + span.newline_ptrs.len) {
      size_t byte_offset = 0;
      if (line - lines_traversed > 0) {
        byte_offset = (size_t)(span.newline_ptrs[line - lines_traversed - 1] -
                               span.contents.begin) +
                      1;
      }
      iterator iter = {this, epoch, i, byte_offset};
      /* TODO: currently allows columns beyond end of the line */
      iter += col;
      return iter;
    }
    lines_traversed += span.newline_ptrs.len;
  }

  unreachable(
      "attempted to create iterator with line/col index outside buffer");
}

/* helper methods */
bool iterator::IsEOF() const {
  return (span_idx + 1 == parent->spans.size() &&
          byte_offset == parent->spans[span_idx].contents.len);
}

/* overloads */

iterator &iterator::operator--() {
  operator-=(1);
  return *this;
}

iterator iterator::operator--(int) {
  iterator old = *this;
  operator--();
  return old;
}

iterator &iterator::operator++() {
  operator+=(1);
  return *this;
}

iterator iterator::operator++(int) {
  iterator old = *this;
  operator++();
  return old;
}

bool operator<(const iterator &lhs, const iterator &rhs) {
  return lhs.span_idx < rhs.span_idx ||
         (lhs.span_idx == rhs.span_idx && lhs.byte_offset < rhs.byte_offset);
}

bool operator>(const iterator &lhs, const iterator &rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const iterator &lhs, const iterator &rhs) {
  return lhs.span_idx < rhs.span_idx ||
         (lhs.span_idx == rhs.span_idx && lhs.byte_offset <= rhs.byte_offset);
}

bool operator>=(const iterator &lhs, const iterator &rhs) {
  return operator<=(rhs, lhs);
}

iterator &iterator::operator+=(size_t num) {
  if (byte_offset + num < parent->spans[span_idx].contents.len) {
    byte_offset += num;
    return *this;
  }

  num -= parent->spans[span_idx].contents.len - byte_offset;
  for (size_t i = span_idx + 1; i < parent->spans.size(); i++) {
    if (num <= parent->spans[i].contents.len) {
      span_idx = i;
      byte_offset = num;
      return *this;
    }
    num -= parent->spans[i].contents.len;
  }

  unreachable("cannot advance past end");
}

iterator operator+(iterator iter, size_t num) {
  iter += num;
  return iter;
}
iterator operator+(size_t num, iterator iter) { return operator+(iter, num); }

iterator &iterator::operator-=(size_t num) {
  /* TODO: probably broken */
  if (num <= byte_offset) {
    byte_offset -= num;
    return *this;
  }

  assert(span_idx > 0);
  // num -= byte_offset;
  for (size_t i = span_idx - 1; i > 0; i--) {
    if (num <= parent->spans[i].contents.len) {
      span_idx = i;
      byte_offset = parent->spans[i].contents.len - num;
      return *this;
    }
    num -= parent->spans[i].contents.len;
  }

  unreachable("cannot advance past end");
}
iterator operator-(iterator iter, size_t num) {
  iter -= num;
  return iter;
}
iterator operator-(size_t num, iterator iter) { return operator-(iter, num); }
ssize_t operator-(iterator, iterator);

const uint8_t &iterator::operator*() const {
  return parent->spans[span_idx].contents[byte_offset];
}