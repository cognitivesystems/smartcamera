#include <glipf/gles-utils/shader-builder.h>

#include <fstream>
#include <iostream>
#include <sstream>


namespace glipf {
namespace gles_utils {


ShaderBuilder::ShaderBuilder(GLenum shaderType) : mIsCompiled(false) {
  mShaderReference = glCreateShader(shaderType);

  if (mShaderReference == 0)
    throw ShaderInitializationError("Failed to create new shader");
}


ShaderBuilder::~ShaderBuilder() {
  /*
   * If the shader hasn't been successfully compiled, the user hasn't
   * got a reference to it and isn't responsible for deleting it
   */
  if (!mIsCompiled)
    glDeleteShader(mShaderReference);
}


ShaderBuilder& ShaderBuilder::appendSourceString(std::string sourceString) {
  mSourceStrings.push_back(sourceString);
  return *this;
}


ShaderBuilder& ShaderBuilder::appendSourceFile(std::string filePath) {
  std::ifstream sourceFile(filePath);

  if (!sourceFile.is_open())
    throw ShaderLoadingError("Could not find source file `" + filePath + "`");

  std::stringstream source;
  source << sourceFile.rdbuf();

  return appendSourceString(source.str());
}


GLuint ShaderBuilder::compile() {
  const GLchar* rawSourceStrings[mSourceStrings.size()];

  for (size_t i = 0; i < mSourceStrings.size(); ++i)
    rawSourceStrings[i] = mSourceStrings[i].c_str();

  glShaderSource(mShaderReference, mSourceStrings.size(), rawSourceStrings,
                 nullptr);
  glCompileShader(mShaderReference);

  GLint compiledSuccessfully;
  glGetShaderiv(mShaderReference, GL_COMPILE_STATUS, &compiledSuccessfully);
  std::string shaderLog = getShaderLog();

  if (!compiledSuccessfully) {
    throw ShaderCompilationError("Failed to compile shader:\n" + shaderLog);
  } else if (shaderLog.length() > 0) {
    std::cout << "Shader compilation log:\n" << shaderLog << "\n";
  }

  mIsCompiled = true;
  return mShaderReference;
}


std::string ShaderBuilder::getShaderLog() const {
  GLint logLength = 0;
  glGetShaderiv(mShaderReference, GL_INFO_LOG_LENGTH, &logLength);

  if (logLength > 1) {
     GLchar shaderLog[sizeof(GLchar) * logLength];
     glGetShaderInfoLog(mShaderReference, logLength, NULL, shaderLog);

     return std::string(shaderLog);
  }

  return std::string();
}


} // end namespace gles_utils
} // end namespace glipf
