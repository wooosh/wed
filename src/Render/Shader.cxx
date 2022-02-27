#include "Shader.hxx"
#include "SDL_opengl_glext.h"
#include "Types.hxx"
#include <iostream>
#include <string>
#include <string_view>

static std::string_view vertex_shader = R"(
  #version 330 core

  in vec2 i_screen_coord;
  in vec2 i_texture_coord;
  in vec4 i_color;
  in float i_depth;

  out vec2 v_texture_pos;
  out vec4 v_color;

  uniform mat4 u_projection_matrix;

  void main() {
      gl_Position = u_projection_matrix * vec4(i_screen_coord, -i_depth, 1.0);
      v_texture_pos = i_texture_coord;
      v_color = i_color;
  }
)";

static std::string_view fragment_shader = R"(
  #version 330 core

  in vec2 v_texture_pos;
  in vec4 v_color;

  out vec4 o_color;

  uniform float texture_saturation;
  uniform sampler2D texture1;
  uniform vec2 u_texture_size;

  void main() {
    /* TODO: fix texture blending */
    o_color = v_color; // * texture(texture1, v_texture_pos/u_texture_size);
  }
)";

static std::string_view subpx_fragment_shader = R"(
  #version 330 core

  in vec2 v_texture_pos;
  in vec4 v_color;

  layout(location = 0, index = 0) out vec4 o_color;
  layout(location = 0, index = 1) out vec4 o_alpha;

  uniform sampler2D texture1;
  uniform vec2 u_texture_size;

  /* TODO: mit license credit */
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

static void CompileShader(GLuint shader_id, std::string_view shader_source) {
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

  CompileShader(vs, vertex_shader);
  glAttachShader(programs.regular, vs);
  glAttachShader(programs.subpx, vs);

  CompileShader(fs, fragment_shader);
  glAttachShader(programs.regular, fs);

  CompileShader(subpx_fs, subpx_fragment_shader);
  glAttachShader(programs.subpx, subpx_fs);

  GLuint progs[2] = {programs.regular, programs.subpx};
  for (size_t i = 0; i < 2; i++) {
    glBindAttribLocation(progs[i], SHADER_SCREEN_COORD, "i_screen_coord");
    glBindAttribLocation(progs[i], SHADER_TEXTURE_COORD, "i_texture_coord");
    glBindAttribLocation(progs[i], SHADER_COLOR, "i_color");
    glBindAttribLocation(progs[i], SHADER_COLOR, "i_depth");
    glLinkProgram(progs[i]);
  }

  return programs;
}