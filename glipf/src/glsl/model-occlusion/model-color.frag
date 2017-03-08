varying vec4 fragColor;


void main(void) {
  gl_FragColor = vec4(fragColor.x, fragColor.yz * 8.0, 1.0);
}
