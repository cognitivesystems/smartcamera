#include <glipf/gles-utils/glsl-program-builder.h>

#include <iostream>


namespace glipf {
namespace gles_utils {


GlslProgramBuilder::GlslProgramBuilder() : mIsLinked(false) {
  mProgramReference = glCreateProgram();

  if (mProgramReference == 0)
    throw GlslProgramInitializationError("Failed to create new program");
}


GlslProgramBuilder::~GlslProgramBuilder() {
  /*
   * If the program hasn't been successfully linked, the user hasn't
   * got a reference to it and isn't responsible for deleting it
   */
  if (!mIsLinked)
    glDeleteProgram(mProgramReference);
}


GlslProgramBuilder& GlslProgramBuilder::attachShader(GLuint shader) {
  glAttachShader(mProgramReference, shader);
  glDeleteShader(shader);
  return *this;
}


GlslProgramBuilder& GlslProgramBuilder::bindAttribLocation(GLuint index,
                                                           const GLchar* name)
{
  glBindAttribLocation(mProgramReference, index, name);
  return *this;
}


GLuint GlslProgramBuilder::link() {
  glLinkProgram(mProgramReference);

  GLint linkedSuccessfully;
  glGetProgramiv(mProgramReference, GL_LINK_STATUS, &linkedSuccessfully);
  std::string programLog = getProgramLog();

  if (!linkedSuccessfully) {
    throw GlslProgramLinkingError("Failed to link program:\n" + programLog);
  } else if (programLog.length() > 0) {
    std::cout << "Program linking log:\n" << programLog << "\n";
  }

  mIsLinked = true;
  return mProgramReference;
}


std::string GlslProgramBuilder::getProgramLog() const {
  GLint logLength = 0;
  glGetProgramiv(mProgramReference, GL_INFO_LOG_LENGTH, &logLength);

  if (logLength > 1) {
     GLchar programLog[sizeof(GLchar) * logLength];
     glGetProgramInfoLog(mProgramReference, logLength, NULL, programLog);

     return std::string(programLog);
  }

  return std::string();
}


} // end namespace gles_utils
} // end namespace glipf
