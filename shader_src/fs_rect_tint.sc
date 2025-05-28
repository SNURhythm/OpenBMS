// fs_rect.sc
$input v_texcoord0

#include "bgfx_shader.sh"

SAMPLER2D(s_texColor, 0);
uniform vec4 u_tintColor;

void main()
{
    vec3 color = vec3(texture2D(s_texColor, v_texcoord0).rgb);
    gl_FragColor = vec4(color * u_tintColor.rgb, 1.0);
}
