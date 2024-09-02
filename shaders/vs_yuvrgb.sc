// vs_yuv_to_rgb.sc
$input a_position, a_texcoord0
$output v_texcoord0

uniform mat4 u_modelViewProj;

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_texcoord0 = a_texcoord0;
}
