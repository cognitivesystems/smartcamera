attribute vec4 vertex;
attribute vec4 vertexColor;

varying vec4 fragColor;

uniform vec2 viewportDimensions;
uniform mat4 projectionMatrix;


void main(void) {
  vec4 projectedPosition = projectionMatrix * vertex;
  vec2 normalizedPosition = vec2(projectedPosition.x / projectedPosition.z,
                                 projectedPosition.y / projectedPosition.z);
  normalizedPosition /= viewportDimensions;

  gl_Position = vec4(-1.0 + normalizedPosition.x * 2.0,
                     -1.0 + normalizedPosition.y * 2.0, 1.0, 1.0);
  fragColor = vertexColor;
}
