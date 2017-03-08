#ifndef shader_builder_h
#define shader_builder_h

#include <GLES2/gl2.h>

#include <stdexcept>
#include <vector>


namespace glipf {
namespace gles_utils {


class ShaderInitializationError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};


class ShaderLoadingError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};


class ShaderCompilationError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};


class ShaderBuilder {
public:
  ShaderBuilder(GLenum shaderType);
  virtual ~ShaderBuilder();

  ShaderBuilder& appendSourceString(std::string sourceString);
  ShaderBuilder& appendSourceFile(std::string filePath);
  GLuint compile();

protected:
  std::string getShaderLog() const;

  bool mIsCompiled;
  GLuint mShaderReference;
  std::vector<std::string> mSourceStrings;
};


} // end namespace gles_utils
} // end namespace glipf

#endif // shader_builder_h
