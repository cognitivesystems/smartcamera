#ifndef processors_processing_result_h
#define processors_processing_result_h

#include <boost/variant/variant.hpp>
#include <GLES2/gl2.h>

#include <vector>
#include <map>


namespace glipf {
namespace processors {


enum ProcessingResultType {
  kTexture, kNumbers
};


typedef boost::variant<GLuint, std::vector<float>> ProcessingResult;
typedef std::map<std::string, ProcessingResult> ProcessingResultSet;


} // end namespace processors
} // end namespace glipf

#endif // processors_processing_result_h
