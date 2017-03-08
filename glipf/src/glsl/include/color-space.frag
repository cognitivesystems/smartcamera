/*
 * Color space conversion functions.
 *
 * This file contains functions that can be used to convert colors
 * between different color spaces. The following conversions are
 * currently supported:
 * - RGB <-> YUV
 * - RGB <-> HSV
 */


const mat3 rgb2yuvMatrix = mat3(
  0.299, 0.587, 0.114,
  -0.14713, -0.28886, 0.436,
  0.615, -0.51499, -0.10001
);

/*
 * Convert a color from RGB to YUV.
 */
vec3 rgb2yuv(in vec3 rgbColor) {
  return rgb2yuvMatrix * rgbColor;
}

const mat3 yuv2rgbMatrix = mat3(
  1, 0, 1.13983,
  1, -0.39465, -0.58060,
  1, 2.03211, 0
);

/*
 * Convert a color from YUV to RGB.
 */
vec3 yuv2rgb(in vec3 yuvColor) {
  return yuv2rgbMatrix * yuvColor;
}

const mat3 rgb2ycocgMatrix = mat3(
  0.25, 0.5, -0.25,
  0.5, 0.0, 0.5,
  0.25, -0.5, -0.25
);

/*
 * Convert a color from RGB to YCoCg.
 */
vec3 rgb2ycocg(in vec3 rgbColor) {
  return rgb2ycocgMatrix * rgbColor;
}

const mat3 ycocg2rgbMatrix = mat3(
  1.0, 1.0, 1.0,
  1.0, 0.0, -1.0,
  -1.0, 1.0, -1.0
);

/*
 * Convert a color from YCoCg to RGB.
 */
vec3 ycocg2rgb(in vec3 ycocgColor) {
  return ycocg2rgbMatrix * ycocgColor;
}

/*
 * Convert a color from RGB to HSV.
 *
 * Implements the algorithm from:
 * http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
 */
vec3 rgb2hsv(in vec3 rgbColor) {
  float K = 0.0;

  if (rgbColor.g < rgbColor.b) {
    rgbColor.gb = rgbColor.bg;
    K = -1.0;
  }

  if (rgbColor.r < rgbColor.g) {
    rgbColor.rg = rgbColor.gr;
    K = -2.0 / 6.0 - K;
  }

  float chroma = rgbColor.r - min(rgbColor.g, rgbColor.b);
  float hue = abs(K + (rgbColor.g - rgbColor.b) / (6.0 * chroma + 1e-20));
  float saturation = chroma / (rgbColor.r + 1e-20);

  return vec3(hue, saturation, rgbColor.r);
}

/*
 * Convert a color from HSV to RGB.
 *
 * Implements the algorithm from: http://www.chilliant.com/rgb2hsv.html
 */
vec3 hsv2rgb(in vec3 hsvColor) {
  float hue = hsvColor.x * 6.0;
  float R = abs(hue - 3.0) - 1.0;
  float G = 2.0 - abs(hue - 2.0);
  float B = 2.0 - abs(hue - 4.0);
  vec3 rgbColor = clamp(vec3(R, G, B), 0.0, 1.0);

  return ((rgbColor - 1.0) * hsvColor.y + 1.0) * hsvColor.z;
}
