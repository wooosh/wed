#include "RenderContext.hxx"
#include "Shader.hxx"
#include "src/Render/Types.hxx"
#include "src/Util/Assert.hxx"
#include <cassert>
#include <cstdint>

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"
#include "SDL_opengl_glext.h"

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam) {
  (void)source, (void)length, (void)id, (void)userParam;
  fprintf(stderr,
          "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity,
          message);
}

void RenderContext::Init(void) {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  programs = LoadShaders();

  /* initialize VAO and VBO */
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glEnableVertexAttribArray(SHADER_SCREEN_COORD);
  glEnableVertexAttribArray(SHADER_TEXTURE_COORD);
  glEnableVertexAttribArray(SHADER_COLOR);
  glEnableVertexAttribArray(SHADER_DEPTH);

  glVertexAttribPointer(SHADER_SCREEN_COORD, 2, GL_SHORT, false, sizeof(Vertex),
                        (void *)offsetof(Vertex, screen_coord));

  glVertexAttribPointer(SHADER_TEXTURE_COORD, 2, GL_SHORT, false,
                        sizeof(Vertex),
                        (void *)offsetof(Vertex, texture_coord));

  glVertexAttribPointer(SHADER_COLOR, 4, GL_UNSIGNED_BYTE, true, sizeof(Vertex),
                        (void *)offsetof(Vertex, color));

  glVertexAttribPointer(SHADER_DEPTH, 1, GL_UNSIGNED_BYTE, true, sizeof(Vertex),
                        (void *)offsetof(Vertex, depth));

  /* default screen color is red */
  glClearColor(0.0, 0.0, 0.0, 1.0);

  /* set up orthographic projection*/
  SDL_GetWindowSize(window, (int *)&win_w, (int *)&win_h);
  UpdateProjection();

  /* enable depth testing so that layering works properly */
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  /* set the first texture unit as active */
  glActiveTexture(GL_TEXTURE0);
}

void RenderContext::UpdateProjection() {
  glViewport(0, 0, win_w, win_h);

  /* depth range */
  float r = max_z;
  float w = win_w;
  float h = win_h;
  /* clang-format off */
  projection_matrix = {{
    {2.0f/w, 0.0f,    0.0f,    0.0f},
    {0.0f,   2.0f/-h, 0.0f,    0.0f},
    {0.0f,   0.0f,    -2.0f/r, 0.0f},
    {-1.0f,  1.0f,    -1.0f,   1.0f}
  }};
  /* clang-format on */
}

void RenderContext::LoadTexture(const uint8_t *data, size_t w, size_t h) {
  texture_size.x = w;
  texture_size.y = h;

  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  /* disable alignment since rgb data is not aligned to 4 byte boundary */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
               data);

  /* use nearest neighbor so subpixels don't get messed up */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

static GLenum TextureFormatToOpenGLEnum(GPUTexture::Format format) {
  switch (format) {
  case GPUTexture::Format::kGrayscale:
    return GL_LUMINANCE;
  case GPUTexture::Format::kRGB:
    return GL_RGB;
  case GPUTexture::Format::kRGBA:
    return GL_RGBA;
  }
  unreachable("unknown enum value");
}

GPUTexture RenderContext::CreateTexture(GPUTexture::Format format, uint w,
                                        uint h, uint8_t *data) {
  GPUTexture texture;
  texture.size = {w, h};
  texture.format = {format};

  glGenTextures(1, &texture.id);
  glBindTexture(GL_TEXTURE_2D, texture.id);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
               TextureFormatToOpenGLEnum(format), GL_UNSIGNED_BYTE, data);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  return texture;
}

void RenderContext::ReallocTexture(GPUTexture &texture, uint w, uint h,
                                   uint8_t *data) {
  glBindTexture(GL_TEXTURE_2D, texture.id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
               TextureFormatToOpenGLEnum(texture.format), GL_UNSIGNED_BYTE,
               data);
}

void RenderContext::CopyIntoTexture(GPUTexture &texture, Rect dst,
                                    uint8_t *data) {
  /* TODO: mismatch between GPuTexture.format and how opengl works, because you
   * can upload data to a texture in any format, regardless of whether or not
   * the two formats match */
  glBindTexture(GL_TEXTURE_2D, texture.id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexSubImage2D(GL_TEXTURE_2D, 0, dst.x, dst.y, dst.w, dst.h,
                  TextureFormatToOpenGLEnum(texture.format), GL_UNSIGNED_BYTE,
                  data);
}

void RenderContext::PushQuad(RenderLayerIdx z, Point dst, Point src, uint w,
                             uint h, vec4<uint8_t> color,
                             std::vector<uint16_t> *indexes) {
  assert((size_t)base_z + (size_t)z < max_z);
  assert(dst.x + w <= UINT16_MAX && dst.y + h <= UINT16_MAX);
  assert(src.x + w <= UINT16_MAX && src.y + h <= UINT16_MAX);

  RenderLayerIdx depth = RENDER_LAYER_IDX_MAX - (base_z + z);

  assert(vertexes.size() < UINT16_MAX);
  uint16_t i = vertexes.size();
  /* clang-format off */
  vertexes.insert(vertexes.end(), {
    {
     {(uint16_t) dst.x, (uint16_t) dst.y},
     {(uint16_t) src.x, (uint16_t) src.y},
     color, depth
    },
    {
     {(uint16_t) (dst.x+w), (uint16_t) dst.y},
     {(uint16_t) (src.x+w), (uint16_t) src.y},
     color, depth
    },
    {
     {(uint16_t)(dst.x+w), (uint16_t)(dst.y+h)},
     {(uint16_t)(src.x+w), (uint16_t)(src.y+h)},
     color, depth
    },
    {
     {(uint16_t) dst.x, (uint16_t) (dst.y+h)},
     {(uint16_t) src.x, (uint16_t) (src.y+h)},
     color, depth
    }
  });
  indexes->insert(indexes->end(), {
    i, (VertexIndex)(i+1), (VertexIndex)(i+2), /* top right triangle */
    i, (VertexIndex)(i+2), (VertexIndex)(i+3), /* bottom left triangle */
  });
  /* clang-format on */
}

void RenderContext::DrawRect(RenderLayerIdx z, Rect dst, Color color) {
  PushQuad(z, dst.top_left(), {0, 0}, dst.w, dst.h, color, &indexes);
}

void RenderContext::DrawTexture(RenderLayerIdx z, Point dst, Rect src) {
  PushQuad(z, dst, src.top_left(), src.w, src.h, {0, 0, 0, 255}, &indexes);
}

void RenderContext::DrawGlyph(RenderLayerIdx z, Point dst, Glyph g,
                              Color color) {
  PushQuad(z, {dst.x + g.padding_x, dst.y - g.padding_y}, {g.x, g.y}, g.w, g.h,
           color, &subpx_indexes);
}

void RenderContext::Commit(void) {
  /* Orphan the last VBO
   * https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming#Buffer_re-specification
   */
  if (last_vbo_size > 0) {
    glBufferData(GL_ARRAY_BUFFER, last_vbo_size, NULL, GL_STREAM_DRAW);
  }
  last_vbo_size = vertexes.size();

  glBufferData(GL_ARRAY_BUFFER, vertexes.size() * sizeof(Vertex),
               vertexes.data(), GL_STREAM_DRAW);

  /* TODO: only clear depth buffer */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glDisable(GL_BLEND);
  // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glUseProgram(programs.regular);
  glUniform2f(glGetUniformLocation(programs.regular, "u_texture_size"),
              texture_size.x, texture_size.y);
  glUniformMatrix4fv(
      glGetUniformLocation(programs.regular, "u_projection_matrix"), 1,
      GL_FALSE, (const GLfloat *)projection_matrix.data);
  glDrawElements(GL_TRIANGLES, indexes.size(), GL_UNSIGNED_SHORT,
                 indexes.data());

  for (auto indices : indices2) {
    printf("vec size %zu\n", indices.indices->size());
    if (indices.indices->size() <= 0)
      continue;
    printf("x\n");
    GLuint program;
    if (indices.subpixel) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
      program = programs.subpx;
    } else {
      glDisable(GL_BLEND);
      program = programs.regular;
    }
    glBindTexture(GL_TEXTURE_2D, indices.texture.id);
    printf("1\n");
    glUseProgram(program);
    printf("2\n");
    glUniform2f(glGetUniformLocation(program, "u_texture_size"),
                indices.texture.size.x, indices.texture.size.y);
    printf("3\n");
    glUniformMatrix4fv(glGetUniformLocation(program, "u_projection_matrix"), 1,
                       GL_FALSE, (const GLfloat *)projection_matrix.data);
    printf("4\n");
    glDrawElements(GL_TRIANGLES, indices.indices->size(), GL_UNSIGNED_SHORT,
                   indices.indices->data());
    printf("5\n");
    indices.indices->clear();
  }

  /* flush to gpu */
  SDL_GL_SwapWindow(window);

  /* reset drawing state */
  base_z = 2;
  vertexes.clear();
  indexes.clear();
  subpx_indexes.clear();
}