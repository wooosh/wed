#include "VertexFormat.hxx"

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"

template <typename T> static GLint GetNumComponents(T x) {
  (void)x;
  if constexpr (std::is_integral<T>::value ||
                std::is_floating_point<T>::value) {
    return 1;
  } else {
    return T::size;
  }
}

#define GenerateTypeToGLEnum(my_type, gl_enum)                                 \
  [[maybe_unused]] static GLenum ScalarToGLTypeEnum(my_type x) {               \
    (void)x;                                                                   \
    return gl_enum;                                                            \
  }                                                                            \
  static_assert(true, "")

GenerateTypeToGLEnum(int8_t, GL_BYTE);
GenerateTypeToGLEnum(uint8_t, GL_UNSIGNED_BYTE);
GenerateTypeToGLEnum(int16_t, GL_SHORT);
GenerateTypeToGLEnum(uint16_t, GL_UNSIGNED_SHORT);
GenerateTypeToGLEnum(int32_t, GL_INT);
GenerateTypeToGLEnum(uint32_t, GL_UNSIGNED_INT);
GenerateTypeToGLEnum(float, GL_FLOAT);
GenerateTypeToGLEnum(double, GL_DOUBLE);

template <typename T> static GLenum ToGLTypeEnum(T x) {
  (void)x;
  if constexpr (std::is_integral<T>::value ||
                std::is_floating_point<T>::value) {
    return ScalarToGLTypeEnum(static_cast<T>(0));
  } else {
    return ScalarToGLTypeEnum(static_cast<typename T::value_type>(0));
  }
}

void BindVBOAttribs(void) {
#define ATTR(attr_name, attr_type, attr_normalized)                            \
  glEnableVertexAttribArray((GLuint)Vertex::Attribute::k##attr_name);
  VERTEX_FORMAT
#undef ATTR

  Vertex v;
#define ATTR(attr_name, attr_type, attr_normalized)                            \
  glVertexAttribPointer((GLuint)Vertex::Attribute::k##attr_name,               \
                        GetNumComponents(v.attr_name),                         \
                        ToGLTypeEnum(v.attr_name), attr_normalized,            \
                        sizeof(Vertex), (void *)offsetof(Vertex, attr_name));
  VERTEX_FORMAT
#undef ATTR
}

template <typename T> static std::string ToGLSLType(T x) {
  if constexpr (std::is_integral<T>::value ||
                std::is_floating_point<T>::value) {
    return "float";
  } else {
    return "vec" + std::to_string(GetNumComponents(x));
  }
}

std::string GenerateVertexShaderHeader(void) {
  std::string header;
  Vertex v;

#define ATTR(attr_name, attr_type, attr_normalized)                            \
  header += "layout(location = " +                                             \
            std::to_string((uint)Vertex::Attribute::k##attr_name) + ") in " +  \
            ToGLSLType(v.attr_name) + " " + #attr_name + ";\n";
  VERTEX_FORMAT
#undef ATTR

  return header;
}