highp float position;

varying vec2 tcoord;

uniform sampler2D tex;
uniform vec2 stepSize;


void main(void) {
  vec4 sum = vec4(0.0);
  vec2 cornerTextureCoordinate = tcoord -
    vec2(TEXEL_WIDTH - 1.0, TEXEL_HEIGHT - 1.0) * stepSize;

  for (float i = 0.0; i < TEXEL_WIDTH; ++i) {
    for (float j = 0.0; j < TEXEL_HEIGHT; ++j) {
      vec4 color = texture2D(tex, cornerTextureCoordinate + vec2(i, j) *
                             2.0 * stepSize);
      sum += color;
    }
  }

  gl_FragColor = sum / (TEXEL_WIDTH * TEXEL_HEIGHT);
}
