varying vec2 tcoord;

uniform sampler2D frameTexture;
uniform sampler2D meanTexture;
uniform sampler2D stdDevTexture;


void main(void) {
  vec4 color = texture2D(frameTexture, tcoord);
  vec3 mean = texture2D(meanTexture, tcoord).xyz - vec3(0.0, 0.5, 0.5);
  vec4 stdDev = texture2D(stdDevTexture, tcoord);

  vec3 delta = rgb2ycocg(color.bgr) - mean;
  mat3 invCovMatrix = mat3(stdDev.x * 16.0, 0.0, 0.0,
                           0.0, stdDev.y * 255.0, 0.0,
                           0.0, 0.0, stdDev.z * 255.0);
  float distance = dot(delta, invCovMatrix * delta);

  if (distance > 0.25) {
    gl_FragColor = color.bgra;
  } else {
    discard;
  }
}
