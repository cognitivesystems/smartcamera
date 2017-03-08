varying vec2 tcoord;

uniform sampler2D tex;
uniform vec3 lowerHsvThreshold;
uniform vec3 upperHsvThreshold;


void main(void) {
  vec3 color = texture2D(tex, tcoord).bgr;
  vec3 hsvColor = rgb2hsv(color);
  bvec3 lowerThreshold = bvec3(step(lowerHsvThreshold, hsvColor));
  bvec3 upperThreshold = bvec3(step(hsvColor, upperHsvThreshold));

  if (all(lowerThreshold) && all(upperThreshold))
    gl_FragColor = vec4(color, 1.0);
  else
    discard;
}
