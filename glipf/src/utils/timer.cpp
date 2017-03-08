#include <glipf/utils/timer.h>


namespace glipf {
namespace utils {


Timer::Timer() {}


Timer& Timer::reset() {
  mTimes.clear();
  return *this;
}


size_t Timer::recordTime() {
  mTimes.emplace_back();
  size_t timeIndex = mTimes.size() - 1;
  clock_gettime(CLOCK_MONOTONIC, &mTimes[timeIndex]);

  return timeIndex;
}


float Timer::getIntervalDuration(size_t startTimeIndex,
                                 size_t endTimeIndex) const
{
  const timespec& startTime = mTimes.at(startTimeIndex);
  const timespec& endTime = mTimes.at(endTimeIndex);

  float duration = endTime.tv_sec - startTime.tv_sec;
  duration += (endTime.tv_nsec - startTime.tv_nsec) / 1e9;

  return duration;
}


float Timer::getIntervalFrequency(size_t startTimeIndex,
                                  size_t endTimeIndex) const
{
  return 1 / getIntervalDuration(startTimeIndex, endTimeIndex);
}


} // end namespace utils
} // end namespace glipf
