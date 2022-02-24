#include "TextBuffer.hxx"
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch2/catch.hpp"

#include "../Util/Assert.hxx"
#include <algorithm>
#include <string_view>

TEST_CASE("text buffer", "[TextBuffer]") {
  TextBuffer tb;

  /* reference string */
  std::string ref_str = R"(
src

src/entry_test.cxx
src/entry_exe.cxx

src/TextBuffer

src/TextBuffer/Edit.cxx
src/TextBuffer/iterator.cxx
src/TextBuffer/ConstView.hxx
src/TextBuffer/AppendBuffer.hxx
src/TextBuffer/TextBuffer.hxx

src/TextBuffer/AppendBufferTest.cxx
src/TextBuffer/TextBufferTest.cxx

src/Util
src/Util/Assert.hxx
src/Util/Assert.cxx
)";

  // insert small file contents
  tb.InsertAt(tb.AtByteOffset(0), ref_str.cbegin(), ref_str.cend());
  REQUIRE(std::equal(ref_str.cbegin(), ref_str.cend(), tb.AtByteOffset(0)));

  // series of small line inserts
  // TODO: test specific edge cases in additional units
  auto line_numbers = {3, 8, 9, 10, 1, 4, 2, 9, 5};
  for (size_t line_idx : line_numbers) {
    std::string_view str = "new line\n";

    /* perform operation on string */
    size_t char_idx = 0;
    size_t line_count = 0;
    while (line_count < line_idx) {
      char_idx = ref_str.find("\n", char_idx + 1);
      REQUIRE(char_idx != std::string::npos);
      line_count++;
    }
    ref_str.insert(char_idx + 1, str);

    /* perform operation on TextBuffer */
    tb.InsertAt(tb.AtLineCol(line_idx, 0), str.cbegin(), str.cend());

    /* compare */
    REQUIRE(std::equal(ref_str.cbegin(), ref_str.cend(), tb.AtByteOffset(0)));
  }
}

TEST_CASE("text buffer insert performance", "[TextBuffer]") {
  const size_t num_iter = GENERATE(100, 1000, 10000);

  BENCHMARK_ADVANCED("insert " + std::to_string(num_iter) +
                     " lines individually at start")
  (Catch::Benchmark::Chronometer meter) {
    std::string_view line = "hello world\n";
    TextBuffer tb;
    auto iter = tb.AtByteOffset(0);

    meter.measure([&tb, &line, &iter, &num_iter] {
      for (size_t i = 0; i < num_iter; i++) {
        tb.InsertAt(iter, line.cbegin(), line.cend());
        iter.epoch++;
      }
    });
  };

  BENCHMARK_ADVANCED("insert " + std::to_string(num_iter) +
                     " lines individually, contigous")
  (Catch::Benchmark::Chronometer meter) {
    std::string_view line = "hello world\n";
    TextBuffer tb;
    auto iter = tb.AtByteOffset(0);

    meter.measure([&tb, &line, &iter, &num_iter] {
      for (size_t i = 0; i < num_iter; i++) {
        tb.InsertAt(iter, line.cbegin(), line.cend());
        iter.span_idx++;
        iter.epoch++;
      }
    });
  };

  BENCHMARK_ADVANCED("insert " + std::to_string(num_iter) +
                     " lines individually, splitting spans")
  (Catch::Benchmark::Chronometer meter) {
    std::string_view line = "hello world\n";
    TextBuffer tb;
    auto iter = tb.AtByteOffset(0);

    meter.measure([&tb, &line, &iter, &num_iter] {
      for (size_t i = 0; i < num_iter; i++) {
        tb.InsertAt(iter, line.cbegin(), line.cend());
        iter++;
        iter.epoch++;
      }
    });
  };
}

TEST_CASE("text buffer iterator performance", "[TextBuffer]") {
  // TODO: randomize data
  const size_t num_nodes = GENERATE(100, 1000, 10000);
  std::string_view line = "hello \nworld\n";
  TextBuffer tb;
  auto iter = tb.AtByteOffset(0);

  for (size_t i = 0; i < num_nodes; i++) {
    tb.InsertAt(iter, line.cbegin(), line.cend());
    iter.epoch++;
  }

  BENCHMARK("start to end - " + std::to_string(num_nodes) + " nodes") {
    auto iter = tb.AtByteOffset(0);
    while (iter.span_idx != num_nodes)
      iter++;
    return iter;
  };
}