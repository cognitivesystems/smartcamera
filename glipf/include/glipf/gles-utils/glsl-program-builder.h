#ifndef glsl_program_builder_h
#define glsl_program_builder_h

#include <GLES2/gl2.h>

#include <stdexcept>


namespace glipf {
namespace gles_utils {


class GlslProgramInitializationError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};


class GlslProgramLinkingError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};


class GlslProgramBuilder {
public:
  GlslProgramBuilder();
  virtual ~GlslProgramBuilder();

  GlslProgramBuilder& attachShader(GLuint shader);
  GlslProgramBuilder& bindAttribLocation(GLuint index, const GLchar* name);
  GLuint link();

protected:
  std::string getProgramLog() const;

  bool mIsLinked;
  GLuint mProgramReference;
};


} // end namespace gles_utils
} // end namespace glipf

#endif // glsl_program_builder_h
