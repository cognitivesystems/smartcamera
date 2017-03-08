varying vec2 tcoord;
uniform sampler2D tex;

void main(void) {
  vec3 color = texture2D(tex, tcoord).xyz;

  // Currently, all color spaces are first converted to RGB
#if defined(INPUT_COLOR_SPACE_BGR)
  color = color.bgr;
  #define INPUT_COLOR_SPACE_RGB
#elif defined(INPUT_COLOR_SPACE_YUV)
  color = yuv2rgb(color);
  #define INPUT_COLOR_SPACE_RGB
#elif defined(INPUT_COLOR_SPACE_HSV)
  color = hsv2rgb(color);
  #define INPUT_COLOR_SPACE_RGB
#endif

  // And then from RGB to the target color space
#if defined(INPUT_COLOR_SPACE_RGB)
  #if defined(OUTPUT_COLOR_SPACE_YUV)
    color = rgb2yuv(color);
  #elif defined(OUTPUT_COLOR_SPACE_HSV)
    color = rgb2hsv(color);
  #elif defined(OUTPUT_COLOR_SPACE_BGR)
    color = color.bgr;
  #endif
#endif

  gl_FragColor = vec4(color, 1);
}
