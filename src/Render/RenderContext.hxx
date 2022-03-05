#pragma once

#include "SDL.h"
#include "Shader.hxx"
#include "Types.hxx"
#include <cstdint>
#include <list>
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
 * - move font api in
 * - move drawRun in
 * - index size type
 * - uint8_t layer everywhere
 * - proper line height detection
 */

/* puts an upper bound on the number of vertices */
typedef uint16_t VertexIndex;

struct RenderContext {
  struct Batch {
    GPUTexture texture;
    bool subpixel;
    std::vector<uint16_t> indices;

    void PushQuad(RenderLayerIdx z, Point dst, Point src, uint w, uint h,
                  vec4<uint8_t> color, std::vector<uint16_t> *indexes);
  };

  SDL_Window *window;
  uint win_w, win_h;

  mat4<float> projection_matrix;

  ShaderPrograms programs;

  std::vector<Vertex> vertexes;
  std::list<Batch> batches;

  Batch *rect_batch;

  GLuint vao;
  GLuint vbo;
  /* used for buffer orphaning */
  GLsizeiptr last_vbo_size;

  /* TODO: change z values to uint8 */
  /* UpdateProjection must be called after changing max z */
  RenderLayerIdx max_z;
  RenderLayerIdx base_z;

  RenderContext(SDL_Window *window)
      : window(window), last_vbo_size(0), max_z(255), base_z(2){};
  RenderContext(RenderContext const &) = delete;
  RenderContext &operator=(RenderContext const &) = delete;

  void Init(void);

  void UpdateProjection();

  Batch *NewBatch(Batch);

  /* TODO: destructor? make members of GPUTexture? refcount/non-movable/ */

  /* data may be NULL for an unitinitalized texture */
  static GPUTexture CreateTexture(GPUTexture::Format, uint w, uint h,
                                  uint8_t *data);
  /* reallocates the storage of a texture, destroying the old contents, reading
   * the new contents from data if non-null
   *
   * data may be NULL for an unitinitalized texture */
  static void ReallocTexture(GPUTexture &, uint w, uint h, uint8_t *data);
  /* data must not be null */
  static void CopyIntoTexture(GPUTexture &, Rect dst, uint8_t *data);

  void PushQuad(Batch *, RenderLayerIdx z, Point dst, Point src, uint w, uint h,
                Color);

  void DrawRect(RenderLayerIdx z, Rect dst, Color color);

  // TODO: vec3 color, no text alpha

  void Commit(void);
};