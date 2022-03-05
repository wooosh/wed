#pragma once

#include "Types.hxx"
#include <string>

#define VERTEX_FORMAT                                                          \
  /* name, type, normalize in shader */                                        \
  ATTR(screen_coord, vec2<uint16_t>, false)                                    \
  ATTR(texture_coord, vec2<uint16_t>, false)                                   \
  ATTR(color, vec4<uint8_t>, true)                                             \
  ATTR(depth, uint8_t, true)

struct Vertex {
#define ATTR(attr_name, attr_type, attr_normalized) attr_type attr_name;
  VERTEX_FORMAT
#undef ATTR

#define ATTR(attr_name, attr_type, attr_normalized) k##attr_name,
  enum class Attribute { VERTEX_FORMAT };
#undef ATTR
};

void BindVBOAttribs(void);
std::string GenerateVertexShaderHeader(void);