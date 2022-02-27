#include "RenderContext.hxx"
#include "Shader.hxx"
#include "src/Render/Types.hxx"
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
  glClearColor(1.0, 0.0, 0.0, 0.0);

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

void RenderContext::PushQuad(uint z, uint dst_x, uint dst_y, uint src_x,
                             uint src_y, uint w, uint h, vec4<uint8_t> color,
                             std::vector<uint16_t> &indexes) {
  assert(base_z + z < max_z);
  assert(dst_x + w <= UINT16_MAX && dst_y + h <= UINT16_MAX);
  assert(src_x + w <= UINT16_MAX && src_y + h <= UINT16_MAX);

  uint8_t depth = (base_z + z);
  /* TODO: use indexed VBO
   * http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-9-vbo-indexing/
   */
  assert(vertexes.size() < UINT16_MAX);
  uint16_t i = vertexes.size();
  /* clang-format off */
  vertexes.insert(vertexes.end(), {
    {
     {(uint16_t) dst_x, (uint16_t) dst_y},
     {(uint16_t) src_x, (uint16_t) src_y},
     color, (uint8_t) depth
    },
    {
     {(uint16_t) (dst_x+w), (uint16_t) dst_y},
     {(uint16_t) (src_x+w), (uint16_t) src_y},
     color, (uint8_t) depth
    },
    {
     {(uint16_t)(dst_x+w), (uint16_t)(dst_y+h)},
     {(uint16_t)(src_x+w), (uint16_t)(src_y+h)},
     color, (uint8_t) depth
    },
    {
     {(uint16_t) dst_x, (uint16_t) (dst_y+h)},
     {(uint16_t) src_x, (uint16_t) (src_y+h)},
     color, (uint8_t) depth
    }
  });
  indexes.insert(indexes.end(), {
    i, (uint16_t)(i+1), (uint16_t)(i+2), /* top right triangle */
    i, (uint16_t)(i+2), (uint16_t)(i+3), /* bottom left triangle */
  });
  /* clang-format on */
}

void RenderContext::DrawRect(uint z, uint x, uint y, uint w, uint h,
                             vec4<uint8_t> color) {
  PushQuad(z, x, y, 0, 0, w, h, color, indexes);
}
/*
void RenderContext::DrawTexture(uint z, uint dst_x, uint dst_y, uint src_x,
                                uint src_y, uint w, uint h);
*/

void RenderContext::DrawGlyph(uint z, uint dst_x, uint dst_y, Glyph g,
                              vec4<uint8_t> color) {
  PushQuad(z, dst_x + g.padding_x, dst_y - g.padding_y, g.x, g.y, g.w, g.h,
           color, subpx_indexes);
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
  glUseProgram(programs.regular);
  glUniform2f(glGetUniformLocation(programs.regular, "u_texture_size"),
              texture_size.x, texture_size.y);
  glUniformMatrix4fv(
      glGetUniformLocation(programs.regular, "u_projection_matrix"), 1,
      GL_FALSE, (const GLfloat *)projection_matrix.data);
  glDrawElements(GL_TRIANGLES, indexes.size(), GL_UNSIGNED_SHORT,
                 indexes.data());

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
  glUseProgram(programs.subpx);
  glUniform2f(glGetUniformLocation(programs.subpx, "u_texture_size"),
              texture_size.x, texture_size.y);
  glUniformMatrix4fv(
      glGetUniformLocation(programs.subpx, "u_projection_matrix"), 1, GL_FALSE,
      (const GLfloat *)projection_matrix.data);

  glDrawElements(GL_TRIANGLES, subpx_indexes.size(), GL_UNSIGNED_SHORT,
                 subpx_indexes.data());

  /* flush to gpu */
  SDL_GL_SwapWindow(window);

  /* reset drawing state */
  base_z = 0;
  vertexes.clear();
  indexes.clear();
  subpx_indexes.clear();
}