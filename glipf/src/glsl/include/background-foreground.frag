/*
 * Functions related to background-foreground segmentation.
 */


/*
 * Subtract background from a frame and return the HSV foreground.
 *
 * @param referenceFrameTexture sampler for the reference (background)
 *                              frame
 * @param frameTexture sampler for the frame that should have its
 *                     background subtracted
 * @param tcoord coordinate at which to subtract background
 * @return HSV color at tcoord in frameTexture after background subtraction
 */
bool subtractBackgroundHsv(in sampler2D referenceFrameTexture,
                           in sampler2D frameTexture, in vec2 tcoord,
                           out vec3 foregroundHsv)
{
  foregroundHsv = rgb2hsv(texture2D(frameTexture, tcoord).bgr);
  vec3 referenceColorHsv = rgb2hsv(texture2D(referenceFrameTexture,
                                             tcoord).bgr);

  float diff = length(vec3(0.0, 1.0, 1.0) *
                      abs(referenceColorHsv - foregroundHsv));

  return diff > 0.25;
}


/*
 * Determine whether a coordinate is in the foreground.
 *
 * @param referenceFrameTexture sampler for the reference (background)
 *                              frame
 * @param frameTexture sampler for the currently processed frame
 * @param tcoord coordinate to analyse
 * @return true it tcoord is part of the foreground, false otherwise
 */
bool isForeground(in sampler2D referenceFrameTexture,
                  in sampler2D frameTexture, in vec2 tcoord)
{
  vec3 colorHsv;

  return subtractBackgroundHsv(referenceFrameTexture, frameTexture, tcoord,
                               colorHsv);
}


/*
 * Subtract background from a frame.
 *
 * @param referenceFrameTexture sampler for the reference (background)
 *                              frame
 * @param frameTexture sampler for the frame that should have its
 *                     background subtracted
 * @param tcoord coordinate at which to subtract background
 * @return color at tcoord in frameTexture after background subtraction
 */
vec3 subtractBackground(in sampler2D referenceFrameTexture,
                        in sampler2D frameTexture, in vec2 tcoord)
{
  if (isForeground(referenceFrameTexture, frameTexture, tcoord)) {
    return texture2D(frameTexture, tcoord).bgr;
  } else {
    return vec3(0.0);
  }
}
