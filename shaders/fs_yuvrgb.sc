$input v_texcoord0
#include <bgfx_shader.sh>
    SAMPLER2D(s_texY, 0);
SAMPLER2D(s_texU, 1);
SAMPLER2D(s_texV, 2);

void main() {
  // Get the full-resolution texture coordinates
  vec2 uv = v_texcoord0;

  // Sample the Y plane directly using the full-resolution coordinates
  float y = texture2D(s_texY, uv).r;

  // TODO: WHY IS THIS WORKING WITHOUT TAKING THE HALF OF THE UV?
  float u = texture2D(s_texU, uv).r - 0.5;
  float v = texture2D(s_texV, uv).r - 0.5;

  // Convert YUV to RGB
  float r = y + 1.402 * v;
  float g = y - 0.344 * u - 0.714 * v;
  float b = y + 1.772 * u;

  // Output the final color
  gl_FragColor = vec4(r, g, b, 1.0);
}