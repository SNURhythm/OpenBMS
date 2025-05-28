$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

void main() {
    vec4 color = texture2D(s_texColor, v_texcoord0);
    
    // Check if the color is exactly black
    if (color.r == 0.0 && color.g == 0.0 && color.b == 0.0) {
        color.a = 0.0; // Make it transparent
    }
    
    gl_FragColor = color;
}