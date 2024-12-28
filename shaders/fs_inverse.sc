// fs_inverse.sc
$input v_texcoord0

#include "bgfx_shader.sh"

SAMPLER2D(s_texColor, 0);

void main()
{
    vec4 color = texture2D(s_texColor, v_texcoord0);
    gl_FragColor = vec4(1.0 - color.rgb, color.a);
}
