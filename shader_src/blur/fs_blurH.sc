// fs_blurH.sc
$input v_texcoord0
#include "../bgfx_shader.sh"

SAMPLER2D(s_texColor, 0);
uniform vec4 u_texelSize; // x=1/width, y=1/height

void main()
{
    const float offsets[9] = {
    -4.0, -3.0, -2.0, -1.0,
        0.0,
        1.0,  2.0,  3.0,  4.0
    };

    const float weights[9] = {
        0.05, 0.09, 0.12, 0.15,
        0.18,
        0.15, 0.12, 0.09, 0.05
    };
    vec2 uv = v_texcoord0;
    vec3 c  = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < 9; i++)
    {
        float4 sampleColor = bgfxTexture2D(s_texColor, uv + float2(offsets[i]*u_texelSize.x, 0.0));
        c += sampleColor.rgb * weights[i];
    }
    gl_FragColor = vec4(c, 1.0);
}
