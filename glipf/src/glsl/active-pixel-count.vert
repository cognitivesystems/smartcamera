attribute vec4 vertex;
varying vec2 tcoord;


void main(void) {
  vec4 pos = vertex;
  gl_Position = pos;
  tcoord = vec2(0.5 + vertex.x * 0.5, 0.5 + vertex.y * 0.5);
}
