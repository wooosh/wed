#include "AppendBuffer.hxx"
#include "catch2/catch.hpp"
#include <algorithm>
#include <string_view>

TEST_CASE("char copying", "[AppendBuffer]") {
  AppendBuffer<char> append_buf;
  {
    std::string_view in = "hello world";
    ConstView<char> out = append_buf.CopyFrom(in.cbegin(), in.cend());
    REQUIRE(std::equal(in.cbegin(), in.cend(), out.cbegin()));
  };
  {
    // test zero length copy
    std::string_view in = "";
    ConstView<char> out = append_buf.CopyFrom(in.cbegin(), in.cend());
    REQUIRE(std::equal(in.cbegin(), in.cend(), out.cbegin()));
  };
  {
    std::string_view in = "new text";
    ConstView<char> out = append_buf.CopyFrom(in.cbegin(), in.cend());
    REQUIRE(std::equal(in.cbegin(), in.cend(), out.cbegin()));
  };
}

TEST_CASE("size_t copying", "[AppendBuffer]") {
  AppendBuffer<size_t> append_buf;
  {
    std::vector<size_t> in = {1, 2, 3, 4};
    ConstView<size_t> out = append_buf.CopyFrom(in.cbegin(), in.cend());
    REQUIRE(std::equal(in.cbegin(), in.cend(), out.cbegin()));
  };
  {
    // test zero length copy
    std::vector<size_t> in = {};
    ConstView<size_t> out = append_buf.CopyFrom(in.cbegin(), in.cend());
    REQUIRE(std::equal(in.cbegin(), in.cend(), out.cbegin()));
  };
  {
    std::vector<size_t> in = {5, 6, 7, 8};
    ConstView<size_t> out = append_buf.CopyFrom(in.cbegin(), in.cend());
    REQUIRE(std::equal(in.cbegin(), in.cend(), out.cbegin()));
  };
}

TEST_CASE("chunking", "[AppendBuffer]") {
  AppendBuffer<char> append_buf;
  std::vector<char> in(append_buf.min_chunk_capacity + 123);
  for (size_t i = 0; i < in.size(); i++) {
    in[i] = (char)i;
  }
  {
    // test one large chunk that does not fit in the min chunk size
    const size_t len_copy = append_buf.min_chunk_capacity + 123;
    ConstView<char> out =
        append_buf.CopyFrom(in.cbegin(), in.cbegin() + len_copy);
    REQUIRE(std::equal(in.cbegin(), in.cbegin() + len_copy, out.cbegin()));
  };
  {
    // insert something that fills the chunk partially
    const size_t len_copy = (append_buf.min_chunk_capacity * 3) / 4;
    ConstView<char> out =
        append_buf.CopyFrom(in.cbegin(), in.cbegin() + len_copy);
    REQUIRE(std::equal(in.cbegin(), in.cbegin() + len_copy, out.cbegin()));
  };
  {
    // insert something that would not overflow the chunk normally, unless there
    // was already data in the chunk
    const size_t len_copy = append_buf.min_chunk_capacity / 2;
    ConstView<char> out =
        append_buf.CopyFrom(in.cbegin(), in.cbegin() + len_copy);
    REQUIRE(std::equal(in.cbegin(), in.cbegin() + len_copy, out.cbegin()));
  };
}
