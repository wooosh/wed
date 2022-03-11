#include "TextBuffer.hxx"
#include <memory>

TextBuffer::TextBuffer() {
  /* Push the EOF span */
  spans.push_back({{nullptr, 0}, {nullptr, 0}});
  num_lines = 0;
  num_bytes = 0;
  epoch = 0;
}

void TextBuffer::NewEpoch() {
  epoch++;
  for (size_t i = 0; i < persisted_iterators.size(); i++) {
    auto iter_ref = persisted_iterators[i];
    if (iter_ref.expired()) {
      persisted_iterators.erase(persisted_iterators.begin() + i);
      i--;
    } else {
      iter_ref.lock()->epoch++;
    }
  }
}

std::shared_ptr<TextBuffer::iterator> TextBuffer::PersistIterator(iterator i) {
  auto new_iter = std::make_shared<iterator>(i);
  persisted_iterators.push_back(new_iter);
  return new_iter;
}

TextBuffer::iterator TextBuffer::begin(void) { return {this, epoch, 0, 0}; }

TextBuffer::iterator TextBuffer::end(void) {
  return {this, epoch, spans.size() - 1, spans[spans.size() - 1].contents.len};
}