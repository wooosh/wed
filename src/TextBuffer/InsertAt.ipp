#pragma once

template <typename ForwardIter>
void TextBuffer::InsertAt(iterator pos, const ForwardIter begin,
                          const ForwardIter end) {
  AssertValidIterator(pos);
  NewEpoch();

  /* append content and line num offsets into buffer */
  Span insert_span;
  insert_span.contents = content_buffer.CopyFrom(begin, end);

  newline_offset_scratch.clear();
  for (size_t i = 0; i < insert_span.contents.len; i++) {
    if (insert_span.contents[i] == '\n') {
      newline_offset_scratch.push_back(insert_span.contents.begin + i);
    }
  }
  insert_span.newline_ptrs = newline_ptr_buffer.CopyFrom(
      newline_offset_scratch.begin(), newline_offset_scratch.end());

  num_lines += insert_span.newline_ptrs.len;
  num_bytes += insert_span.contents.len;

  /* update spans array */
  Span pos_span = spans[pos.span_idx];

  /* replace current span if the previous data and the inserted data are
   * contigous in memory and in the text buffer */
  if (pos.span_idx > 0) {
    Span &prev = spans[pos.span_idx - 1];
    printf("insert %zu %zu\n", pos.byte_offset, pos_span.contents.len);
    if (pos.byte_offset == 0 &&
        prev.contents.cend() == insert_span.contents.begin &&
        prev.newline_ptrs.cend() == insert_span.newline_ptrs.begin) {
      prev.contents.len += insert_span.contents.len;
      prev.newline_ptrs.len += insert_span.contents.len;
      return;
    }
  }

  if (pos.byte_offset == 0) {
    spans.insert(spans.begin() + pos.span_idx, insert_span);

    /* update cursors */
    size_t first_invalid_span_idx = pos.span_idx;
    for (auto i_ptr : persisted_iterators) {
      auto i = i_ptr.lock();
      if (i->span_idx >= first_invalid_span_idx) {
        i->span_idx++;
      }
    }
  } else if (pos.byte_offset == pos_span.contents.len) {
    spans.insert(spans.begin() + pos.span_idx + 1, insert_span);

    /* update cursors */
    size_t first_invalid_span_idx = pos.span_idx + 1;
    for (auto i_ptr : persisted_iterators) {
      auto i = i_ptr.lock();
      if (i->span_idx >= first_invalid_span_idx) {
        i->span_idx++;
      }
    }
  } else {
    /* cursor neither at beginning or end of span, so split the current span */
    size_t num_newlines_before_split = 0;
    for (size_t i = 0; i < pos_span.newline_ptrs.len; i++) {
      if (pos_span.newline_ptrs[i] >=
          pos.byte_offset + pos_span.contents.begin) {
        break;
      }
      num_newlines_before_split++;
    }
    Span left = {{pos_span.contents.begin, pos.byte_offset},
                 {pos_span.newline_ptrs.begin, num_newlines_before_split}};

    Span right = {{pos_span.contents.begin + pos.byte_offset,
                   pos_span.contents.len - pos.byte_offset},
                  {pos_span.newline_ptrs.begin + num_newlines_before_split,
                   pos_span.newline_ptrs.len - num_newlines_before_split}};

    spans[pos.span_idx] = left;
    spans.insert(spans.begin() + pos.span_idx + 1, {insert_span, right});

    /* update cursors */
    size_t first_invalid_span_idx = pos.span_idx;
    size_t split_offset = pos.byte_offset;
    for (auto i_ptr : persisted_iterators) {
      auto i = i_ptr.lock();
      if (i->span_idx == first_invalid_span_idx) {
        if (i->byte_offset >= split_offset) {
          i->span_idx += 2;
          i->byte_offset -= split_offset;
        }
      } else if (i->span_idx > first_invalid_span_idx) {
        i->span_idx += 2;
      }
    }
  }
}