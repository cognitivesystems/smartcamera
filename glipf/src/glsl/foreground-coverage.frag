varying vec4 vColor;
varying vec2 tcoord;

uniform sampler2D tex;


void main(void) {
  if (texture2D(tex, tcoord).a != 0.0) {
    if (vColor.r > 0.0) {
      gl_FragColor = vec4(1.0, 1.0, 0.0, 0.0);
    } else {
      gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
    }
  } else {
    if (vColor.r > 0.0) {
      gl_FragColor = vec4(1.0, 0.0, 0.0, 0.0);
    } else {
      gl_FragColor = vec4(0.0, 0.0, 1.0, 0.0);
    }
  }
}
