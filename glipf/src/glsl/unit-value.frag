precision highp float;

#define UNIT_VALUE 1.0 / 256.0

#define STEP_SIZE_X (1.0 / 640.0)
#define STEP_SIZE_Y (1.0 / 480.0)
#define HALF_STEP_SIZE_X (1.0 / 1280.0)
#define HALF_STEP_SIZE_Y (1.0 / 960.0)

varying vec3 pointCoord;


float rand(vec2 co){
  return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}


void main(void) {
  float randVal = rand(pointCoord.xy);

  if (randVal < 0.25)
    gl_FragColor = vec4(UNIT_VALUE, 0.0, 0.0, 0.0);
  else if (randVal < 0.5)
    gl_FragColor = vec4(0.0, UNIT_VALUE, 0.0, 0.0);
  else if (randVal < 0.75)
    gl_FragColor = vec4(0.0, 0.0, UNIT_VALUE, 0.0);
  else
    gl_FragColor = vec4(0.0, 0.0, 0.0, UNIT_VALUE);
}
