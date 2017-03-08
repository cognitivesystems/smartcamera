varying vec2 tcoord;
uniform sampler2D tex;
uniform sampler2D referenceFrameTexture;


void main(void) {
  if (isForeground(referenceFrameTexture, tex, tcoord))
    gl_FragColor = texture2D(tex, tcoord);
  else
    discard;
}
