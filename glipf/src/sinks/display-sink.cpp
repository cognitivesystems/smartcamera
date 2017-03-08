#include <glipf/sinks/display-sink.h>

#include <glipf/gles-utils/shader-builder.h>
#include <glipf/gles-utils/glsl-program-builder.h>

#include <boost/variant/get.hpp>

#include <iomanip>
#include <iostream>


#define assertNoGlError() assert(glGetError() == GL_NO_ERROR)


namespace glipf {
namespace sinks {


enum VertexAttributeLocations : GLuint {
  kPosition = 0
};


DisplaySink::DisplaySink(uint32_t width, uint32_t height)
  : mWidth(width)
  , mHeight(height)
  , mQuadVertexBuffer(0)
  , mGlslProgram(0)
{
  // Upload vertex data to a buffer
  static const GLfloat vertex_data[] = {
    -1.0, -1.0, 1.0, 1.0,
    1.0, -1.0, 1.0, 1.0,
    1.0, 1.0, 1.0, 1.0,
    -1.0, 1.0, 1.0, 1.0
  };

  glGenBuffers(1, &mQuadVertexBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mQuadVertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data,
               GL_STATIC_DRAW);

  mGlslProgram = gles_utils::GlslProgramBuilder()
    .attachShader(gles_utils::ShaderBuilder(GL_VERTEX_SHADER)
                    .appendSourceFile("glsl/display.vert")
                    .compile())
    .attachShader(gles_utils::ShaderBuilder(GL_FRAGMENT_SHADER)
                    .appendSourceFile("glsl/noop.frag")
                    .compile())
    .bindAttribLocation(VertexAttributeLocations::kPosition, "vertex")
    .link();

  glUseProgram(mGlslProgram);
  glUniform1i(glGetUniformLocation(mGlslProgram, "tex"), 0);
}


void DisplaySink::drawFullscreenQuad() {
  glBindBuffer(GL_ARRAY_BUFFER, mQuadVertexBuffer);
  glVertexAttribPointer(VertexAttributeLocations::kPosition, 4, GL_FLOAT,
                        GL_FALSE, 0, 0);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  assertNoGlError();
}

void DisplaySink::displayTextures(size_t textureCount,
                                  const processors::ProcessingResultSet& resultSet)
{
  unsigned int horizontalCellCount = 1, verticalCellCount = 1;

  while (horizontalCellCount * verticalCellCount < textureCount) {
    horizontalCellCount++;

    if (horizontalCellCount * verticalCellCount < textureCount)
      verticalCellCount++;
  }

  size_t cellWidth = mWidth / horizontalCellCount;
  size_t cellHeight = mHeight / verticalCellCount;
  unsigned int i = 0, j = 0;

  glEnableVertexAttribArray(VertexAttributeLocations::kPosition);

  for (auto const& entry : resultSet) {
    if (entry.second.which() == processors::ProcessingResultType::kTexture) {
      glViewport(j * cellWidth, i * cellHeight, cellWidth, cellHeight);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glUseProgram(mGlslProgram);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, boost::get<GLuint>(entry.second));
      drawFullscreenQuad();

      if (++j >= horizontalCellCount) {
        i++;
        j = 0;
      }
    }
  }

  glDisableVertexAttribArray(VertexAttributeLocations::kPosition);
}


void DisplaySink::send(const processors::ProcessingResultSet& resultSet) {
  size_t textureResultCount = 0;

  for (auto const& entry : resultSet) {
    switch (entry.second.which()) {
      case processors::ProcessingResultType::kTexture:
        textureResultCount++;
        break;
      case processors::ProcessingResultType::kNumbers:
        std::cout << entry.first << ": " << std::fixed << std::setprecision(2);

        for (auto number : boost::get<std::vector<float>>(entry.second)) {
          std::cout << number << ", ";
        }

        std::cout << "\n";
        break;
    }
  }

  if (textureResultCount > 0)
    displayTextures(textureResultCount, resultSet);
}


} // end namespace sinks
} // end namespace glipf
