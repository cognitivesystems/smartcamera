varying vec2 tcoord;
varying vec4 vColor;

uniform sampler2D tex;


void main(void) {
  vec3 colorHsv = rgb2hsv(texture2D(tex, tcoord).rgb);

  if (colorHsv.b == 0.0 || colorHsv.b == 1.0)
    discard;
  else
    gl_FragColor = 0.5 * vColor * vec4(colorHsv.rg, colorHsv.rg);
}
