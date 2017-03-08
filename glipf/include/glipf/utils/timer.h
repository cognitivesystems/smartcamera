#ifndef timer_h
#define timer_h

#include <ctime>
#include <vector>


namespace glipf {
namespace utils {

class Timer {
public:
  Timer();

  Timer& reset();
  size_t recordTime();
  float getIntervalDuration(size_t startTimeIndex, size_t endTimeIndex) const;
  float getIntervalFrequency(size_t startTimeIndex, size_t endTimeIndex) const;

protected:
  std::vector<timespec> mTimes;
};

} // end namespace utils
} // end namespace glipf

#endif // timer_h
