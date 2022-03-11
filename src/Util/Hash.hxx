#pragma once

#include "Assert.hxx"
#include "xxhash.h"
#include <cassert>

typedef XXH64_hash_t Hash;

template <class...> constexpr std::false_type always_false{};

struct Hasher {
  XXH64_state_t *state;

  Hasher(void) {
    state = XXH64_createState();
    assume(state != nullptr, "xxh64 failed to create state");
    XXH_errorcode err = XXH64_reset(state, 0);
    assume(err != XXH_ERROR, "xxh64 failed to reset state");
  }

  void UpdateHash(void *data, size_t len) {
    XXH_errorcode err = XXH64_update(state, data, len);
    assert(err != XXH_ERROR);
  }

  operator Hash() { return XXH64_digest(state); }

  template <typename T> Hasher &operator()(T v) {
    if constexpr (std::is_integral<T>::value) {
      UpdateHash(&v, sizeof(T));
    } else {
      static_assert(always_false<T>, "hash unimplemented for type");
    }

    return *this;
  }
};
