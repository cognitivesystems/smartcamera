attribute vec4 vertex;
varying vec2 tcoord;

void main(void) {
  gl_Position = vertex;
  tcoord = vec2(0.5 + vertex.x * 0.5, 0.5 - vertex.y * 0.5);
}
