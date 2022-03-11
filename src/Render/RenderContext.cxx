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
#ifndef __APPLE__
  /*glDebugMessageCallback(MessageCallback, 0);*/
#endif /* __APPLE__ */

  programs = LoadShaders();

  /* initialize VAO and VBO */
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  BindVBOAttribs();

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

  static uint8_t backing_texture[4] = {0xff, 0xff, 0xff, 0xff};
  rect_batch = NewBatch(
      CreateTexture(GPUTexture::Format::kGrayscale, 1, 1, backing_texture),
      false);
}

RenderContext::Batch *RenderContext::NewBatch(GPUTexture t, bool subpixel) {
  batches.emplace_back(Batch(t, subpixel));
  return &batches.back();
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

void RenderContext::PushQuad(Batch *batch, RenderLayerIdx z, Point dst,
                             Point src, uint w, uint h, Color color) {
  assert((size_t)base_z + (size_t)z < max_z);
  // assert(dst.x + w <= UINT16_MAX && dst.y + h <= UINT16_MAX);
  // assert(src.x + w <= UINT16_MAX && src.y + h <= UINT16_MAX);

  RenderLayerIdx depth = RENDER_LAYER_IDX_MAX - (base_z + z);

  assert(vertexes.size() < UINT16_MAX);
  uint16_t i = vertexes.size();
  /* clang-format off */
  vertexes.insert(vertexes.end(), {
    {
     {(int16_t) dst.x, (int16_t) dst.y},
     {(uint16_t) src.x, (uint16_t) src.y},
     color, depth
    },
    {
     {(int16_t) (dst.x+w), (int16_t) dst.y},
     {(uint16_t) (src.x+w), (uint16_t) src.y},
     color, depth
    },
    {
     {(int16_t)(dst.x+w), (int16_t)(dst.y+h)},
     {(uint16_t)(src.x+w), (uint16_t)(src.y+h)},
     color, depth
    },
    {
     {(int16_t) dst.x, (int16_t) (dst.y+h)},
     {(uint16_t) src.x, (uint16_t) (src.y+h)},
     color, depth
    }
  });
  batch->indices.insert(batch->indices.end(), {
    i, (VertexIndex)(i+1), (VertexIndex)(i+2), /* top right triangle */
    i, (VertexIndex)(i+2), (VertexIndex)(i+3), /* bottom left triangle */
  });
  /* clang-format on */
}

void RenderContext::DrawRect(RenderLayerIdx z, Rect dst, Color color) {
  PushQuad(rect_batch, z, dst.top_left(), {0, 0}, dst.w, dst.h, color);
}

void RenderContext::Commit(void) {
  if (vertexes.size() == 0)
    return;

  /* Orphan the last VBO
   * https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming#Buffer_re-specification
   */
  if (last_vbo_size > 0) {
    glBufferData(GL_ARRAY_BUFFER, last_vbo_size, NULL, GL_STREAM_DRAW);
  }
  last_vbo_size = vertexes.size();

  glBufferData(GL_ARRAY_BUFFER, vertexes.size() * sizeof(Vertex),
               vertexes.data(), GL_STREAM_DRAW);

  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);

  glEnable(GL_BLEND);

  for (auto &batch : batches) {
    if (batch.indices.size() <= 0)
      continue;
    GLuint program;
    if (batch.subpixel) {
      glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
      program = programs.subpx;
    } else {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      program = programs.regular;
    }
    glBindTexture(GL_TEXTURE_2D, batch.texture.id);

    /* TODO: if last_program != program */
    glUseProgram(program);
    glUniform2f(glGetUniformLocation(program, "u_texture_size"),
                batch.texture.size.x, batch.texture.size.y);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_projection_matrix"), 1,
                       GL_FALSE, (const GLfloat *)projection_matrix.data);

    glDrawElements(GL_TRIANGLES, batch.indices.size(), GL_UNSIGNED_SHORT,
                   batch.indices.data());
  }

  /* flush to gpu */
  SDL_GL_SwapWindow(window);

  for (auto &batch : batches) {
    batch.indices.clear();
  }

  /* reset drawing state */
  base_z = 2;
  vertexes.clear();
}
