precision highp float;

attribute vec3 vertex;

uniform ivec3 gridDimensions;
uniform sampler2D tex;

varying vec3 pointCoord;


void main(void) {
  vec4 color = texture2D(tex, vertex.xy);
  vec2 saturationValue;

  if (vertex.z == 0.0)
    saturationValue = color.rg;
  else
    saturationValue = color.ba;

  vec2 multiplier = vec2(2.0) / vec2(gridDimensions.x * gridDimensions.z,
                                     gridDimensions.y);

  if (saturationValue.x != 0.0 || saturationValue.y != 0.0) {
    saturationValue = clamp(saturationValue * multiplier,
                            vec2(0.0), multiplier - vec2(0.01, 0.01));
    vec2 offset = vec2(2.0 * floor(vertex.x * float(gridDimensions.x)) +
                           vertex.z,
                       floor(vertex.y * float(gridDimensions.y)));
    vec2 bucket = multiplier * offset - vec2(1.0) + saturationValue;
    pointCoord = vertex;

    gl_Position = vec4(bucket, 1.0, 1.0);
    gl_PointSize = 1.0;
  }
}
