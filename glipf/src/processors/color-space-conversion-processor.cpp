#include <glipf/processors/color-space-conversion-processor.h>

#include <glipf/gles-utils/shader-builder.h>
#include <glipf/gles-utils/glsl-program-builder.h>


using std::string;
using std::vector;
using glipf::sources::ColorSpace;


namespace glipf {
namespace processors {


enum VertexAttributeLocations : GLuint {
  kPosition = 0
};


ColorSpaceConversionProcessor::ColorSpaceConversionProcessor(const sources::FrameProperties& frameProperties,
                                                             ColorSpace from,
                                                             ColorSpace to)
  : GlesProcessor(frameProperties)
  , mGlslProgram(0)
{
  gles_utils::ShaderBuilder fragmentShaderBuilder(GL_FRAGMENT_SHADER);

  if (from == to)
    fragmentShaderBuilder.appendSourceFile("glsl/noop.frag");
  else {
    fragmentShaderBuilder.appendSourceFile("glsl/include/color-space.frag");
    string colorSpaceDefines;

    switch (from) {
      case ColorSpace::BGR:
        colorSpaceDefines += "#define INPUT_COLOR_SPACE_BGR\n";
        break;
      case ColorSpace::RGB:
        colorSpaceDefines += "#define INPUT_COLOR_SPACE_RGB\n";
        break;
      case ColorSpace::YUV:
        colorSpaceDefines += "#define INPUT_COLOR_SPACE_YUV\n";
        break;
      case ColorSpace::HSV:
        colorSpaceDefines += "#define INPUT_COLOR_SPACE_HSV\n";
        break;
    }

    switch (to) {
      case ColorSpace::BGR:
        colorSpaceDefines += "#define OUTPUT_COLOR_SPACE_BGR\n";
        break;
      case ColorSpace::RGB:
        colorSpaceDefines += "#define OUTPUT_COLOR_SPACE_RGB\n";
        break;
      case ColorSpace::YUV:
        colorSpaceDefines += "#define OUTPUT_COLOR_SPACE_YUV\n";
        break;
      case ColorSpace::HSV:
        colorSpaceDefines += "#define OUTPUT_COLOR_SPACE_HSV\n";
        break;
    }

    fragmentShaderBuilder.appendSourceString(colorSpaceDefines)
      .appendSourceFile("glsl/color-space-conversion.frag");
  }

  mGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/standard.vert")
                    .compile())
    .attachShader(fragmentShaderBuilder.compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
    .link();

  glUseProgram(mGlslProgram);
  glUniform1i(glGetUniformLocation(mGlslProgram, "tex"), 0);

  std::tie(mResultTexture, mResultFbo) =
      generateTextureBackedFbo(frameProperties.dimensions());
  mResultSet["color_space_converted_texture"] = mResultTexture;
}


ColorSpaceConversionProcessor::~ColorSpaceConversionProcessor() {
  glDeleteProgram(mGlslProgram);
  glDeleteFramebuffers(1, &mResultFbo);
  glDeleteTextures(1, &mResultTexture);
}


const ProcessingResultSet& ColorSpaceConversionProcessor::process(GLuint frameTexture) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, frameTexture);

  glBindFramebuffer(GL_FRAMEBUFFER, mResultFbo);
  glViewport(0, 0, mFrameProperties.dimensions().first,
             mFrameProperties.dimensions().second);
  glUseProgram(mGlslProgram);

  glEnableVertexAttribArray(VertexAttributeLocations::kPosition);
  drawFullscreenQuad(VertexAttributeLocations::kPosition);
  glDisableVertexAttribArray(VertexAttributeLocations::kPosition);

  return mResultSet;
}


} // end namespace processors
} // end namespace glipf
