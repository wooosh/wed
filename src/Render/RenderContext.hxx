#pragma once

#include "GlyphAtlas.hxx"
#include "SDL.h"
#include "Shader.hxx"
#include "Types.hxx"
#include <cstdint>
#include <sys/types.h>
#include <vector>

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl.h"

/* TODO:
 * - multiple textures
 * - better glyph atlas handling
 * - frametimes
 * - layout directives?
 * - fix alpha blending
 */

struct RenderContext {
  SDL_Window *window;
  uint win_w, win_h;

  mat4<float> projection_matrix;

  ShaderPrograms programs;

  GLuint texture;
  vec2<uint16_t> texture_size;

  std::vector<Vertex> vertexes;
  std::vector<uint16_t> indexes;
  std::vector<uint16_t> subpx_indexes;

  GLuint vao;
  GLuint vbo;
  /* used for buffer orphaning */
  GLsizeiptr last_vbo_size;

  /* TODO: change z values to uint8 */
  /* UpdateProjection must be called after changing max z */
  uint8_t max_z;
  uint base_z;

  RenderContext(SDL_Window *window)
      : window(window), last_vbo_size(0), max_z(255), base_z(0){};
  RenderContext(RenderContext const &) = delete;
  RenderContext &operator=(RenderContext const &) = delete;

  void Init(void);

  void UpdateProjection();

  /* assumes RGB format TODO: fix */
  void LoadTexture(const uint8_t *data, size_t w, size_t h);

  void PushQuad(uint z, uint dst_x, uint dst_y, uint src_x, uint src_y, uint w,
                uint h, vec4<uint8_t> color, std::vector<uint16_t> &indexes);

  void DrawRect(uint z, uint x, uint y, uint w, uint h, vec4<uint8_t> color);
  void DrawTexture(uint z, uint dst_x, uint dst_y, uint src_x, uint src_y,
                   uint w, uint h);
  // TODO: vec3 color, no text alpha
  void DrawGlyph(uint z, uint dst_x, uint dst_y, Glyph, vec4<uint8_t> color);

  void Commit(void);
};