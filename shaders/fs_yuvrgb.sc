// fs_yuv_to_rgb.sc
$input v_texcoord0

SAMPLER2D(s_texY, 0);
SAMPLER2D(s_texU, 1);
SAMPLER2D(s_texV, 2);

void main() {
    // Sample the Y, U, V components
    float y = texture2D(s_texY, v_texcoord0).r;

    // The U and V textures are half the resolution of the Y texture
    vec2 uvCoord = v_texcoord0 * vec2(0.5, 0.5);
    float u = texture2D(s_texU, uvCoord).r - 0.5;
    float v = texture2D(s_texV, uvCoord).r - 0.5;

    // Convert YUV to RGB
    float r = y + 1.402 * v;
    float g = y - 0.344 * u - 0.714 * v;
    float b = y + 1.772 * u;

    // Output the final color
    gl_FragColor = vec4(r, g, b, 1.0);
}
