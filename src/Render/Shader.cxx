#include "Shader.hxx"
#include "SDL_opengl_glext.h"
#include "Types.hxx"
#include <iostream>
#include <string>
#include <string_view>

static std::string vertex_shader = R"(
  out vec2 v_texture_pos;
  out vec4 v_color;

  uniform mat4 u_projection_matrix;

  void main() {
      gl_Position = u_projection_matrix * vec4(screen_coord, -depth, 1.0);
      v_texture_pos = texture_coord;
      v_color = color;
  }
)";

static std::string fragment_shader = R"(
  #version 330 core

  in vec2 v_texture_pos;
  in vec4 v_color;

  out vec4 o_color;
  out vec4 o_alpha;

  uniform float texture_saturation;
  uniform sampler2D texture1;
  uniform vec2 u_texture_size;

  void main() {
    /* TODO: fix texture blending */
    o_color = v_color; // *
    //o_color = texture(texture1, v_texture_pos/u_texture_size);
    o_alpha = vec4(1, 1, 1, 1);
  }
)";

static std::string subpx_fragment_shader = R"(
  /*
  * Copyright (c) 2020 Chad Brokaw
  *
  * Permission is hereby granted, free of charge, to any
  * person obtaining a copy of this software and associated
  * documentation files (the "Software"), to deal in the
  * Software without restriction, including without
  * limitation the rights to use, copy, modify, merge,
  * publish, distribute, sublicense, and/or sell copies of
  * the Software, and to permit persons to whom the Software
  * is furnished to do so, subject to the following
  * conditions:
  *
  * The above copyright notice and this permission notice
  * shall be included in all copies or substantial portions
  * of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
  * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
  * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
  * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
  * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  * DEALINGS IN THE SOFTWARE.
  */

  #version 330 core

  in vec2 v_texture_pos;
  in vec4 v_color;

  layout(location = 0, index = 0) out vec4 o_color;
  layout(location = 0, index = 1) out vec4 o_alpha;

  uniform sampler2D texture1;
  uniform vec2 u_texture_size;

  const float gamma_lut[256] = float[256](
    0.000, 0.058, 0.117, 0.175, 0.234, 0.293, 0.353, 0.413, 0.473, 0.534, 0.597, 0.661, 0.727, 0.797, 0.876, 1.000, 
    0.000, 0.021, 0.082, 0.143, 0.203, 0.264, 0.325, 0.386, 0.448, 0.510, 0.572, 0.635, 0.700, 0.766, 0.836, 1.000, 
    0.000, 0.000, 0.034, 0.098, 0.161, 0.224, 0.287, 0.350, 0.413, 0.475, 0.538, 0.601, 0.665, 0.729, 0.793, 1.000, 
    0.000, 0.000, 0.000, 0.033, 0.099, 0.165, 0.231, 0.296, 0.360, 0.425, 0.489, 0.552, 0.616, 0.679, 0.741, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000, 
    0.000, 0.092, 0.180, 0.264, 0.345, 0.422, 0.496, 0.566, 0.633, 0.696, 0.756, 0.812, 0.864, 0.913, 0.958, 1.000 
  );

  float luma(vec4 color) {
    return color.x * 0.25 + color.y * 0.72 + color.z * 0.075;
  }
  float gamma_alpha(float luma, float alpha) {
      int luma_index = int(clamp(luma * 15.0, 0.0, 15.0));
      int alpha_index = int(clamp(alpha * 15.0, 0.0, 15.0));
      return gamma_lut[luma_index * 15 + alpha_index];
  }
  vec4 subpx_gamma_alpha(vec4 color, vec4 mask) {
      float l = luma(color);
      return vec4(
          gamma_alpha(l, mask.x * color.a),
          gamma_alpha(l, mask.y * color.a),
          gamma_alpha(l, mask.z * color.a),
          1.0
      );
  }
  const float GAMMA = 1.0 / 1.2;
  const float CONTRAST = 0.8;
  float gamma_correct(float luma, float alpha, float gamma, float contrast) {
      float inverse_luma = 1.0 - luma;
      float inverse_alpha = 1.0 - alpha;
      float g = pow(luma * alpha + inverse_luma * inverse_alpha, gamma);
      float a = (g - inverse_luma) / (luma - inverse_luma);
      a = a + ((1.0 - a) * contrast * a);
      return clamp(a, 0.0, 1.0);
  }
  vec4 gamma_correct_subpx(vec4 color, vec4 mask) {
      float l = luma(color);
      float inverse_luma = 1.0 - l;
      float gamma = mix(1.0 / 1.2, 1.0 / 2.4, inverse_luma);
      float contrast = mix(0.1, 0.8, inverse_luma);
      return vec4(
          gamma_correct(l, mask.x * color.a, gamma, contrast),
          gamma_correct(l, mask.y * color.a, gamma, contrast),
          gamma_correct(l, mask.z * color.a, gamma, contrast),
          1.0
    );
  }

  void main() {
    o_color = vec4(v_color.xyz, 1.0);
    o_alpha = gamma_correct_subpx(v_color, texture(texture1, v_texture_pos/u_texture_size));
  }
)";

static void CompileShader(GLuint shader_id, std::string &shader_source) {
  const char *shader_data = shader_source.data();
  const int shader_len = shader_source.size();

  glShaderSource(shader_id, 1, &shader_data, &shader_len);
  glCompileShader(shader_id);

  GLint status;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint error_len = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &error_len);

    std::string error_msg(error_len, '#');
    glGetShaderInfoLog(shader_id, error_len, &error_len, error_msg.data());
    std::cerr << error_msg << "\n";
    std::abort();
  }
}

ShaderPrograms LoadShaders(void) {
  ShaderPrograms programs = {glCreateProgram(), glCreateProgram()};

  GLuint vs, fs, subpx_fs;
  vs = glCreateShader(GL_VERTEX_SHADER);
  fs = glCreateShader(GL_FRAGMENT_SHADER);
  subpx_fs = glCreateShader(GL_FRAGMENT_SHADER);

  std::string vert_shader =
      "#version 330 core\n" + GenerateVertexShaderHeader() + vertex_shader;
  std::cerr << vert_shader << "\n";
  CompileShader(vs, vert_shader);
  glAttachShader(programs.regular, vs);
  glAttachShader(programs.subpx, vs);

  CompileShader(fs, fragment_shader);
  glAttachShader(programs.regular, fs);
  glLinkProgram(programs.regular);

  CompileShader(subpx_fs, subpx_fragment_shader);
  glAttachShader(programs.subpx, subpx_fs);
  glLinkProgram(programs.subpx);

  return programs;
}