attribute vec4 vertex;
attribute vec4 vertexColor;
attribute vec2 cellOffset;

varying vec4 vColor;
varying vec2 tcoord;

uniform vec2 viewportDimensions;
uniform mat4 projectionMatrix;


void main(void) {
  vec4 projectedPosition = projectionMatrix * vertex;
  vec2 normalizedPosition = vec2(projectedPosition.x / projectedPosition.z,
                                 projectedPosition.y / projectedPosition.z);
  normalizedPosition /= viewportDimensions;

  vec2 cellPosition = vec2(0.5) * normalizedPosition - vec2(1.0) +
                      vec2(0.5) * cellOffset;

  gl_Position = vec4(cellPosition, 1, 1);
  vColor = vertexColor;
  tcoord = normalizedPosition;
}
