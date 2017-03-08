#ifndef sinks_sink_h
#define sinks_sink_h

#include "../processors/processing-result.h"


namespace glipf {
namespace sinks {

class Sink {
public:
  virtual ~Sink() {};

  virtual void send(const processors::ProcessingResultSet& resultSet) = 0;
};

} // end namespace sinks
} // end namespace glipf

#endif // sinks_sink_h
