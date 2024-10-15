#ifndef _ESTIMATE_H_
#define _ESTIMATE_H_
#include <chrono>

/**
 * @brief Computes how many time is needed to complete progress to 100%
 * @param startTime timestamp right before progress starts.
 * @param progress current progress
 * @return time in miliseconds that needed to complete progress.
 */
float estimateProgress(std::chrono::high_resolution_clock::time_point &startTime, float progress) {
  const auto now = std::chrono::high_resolution_clock::now();
  const std::chrono::duration<float, std::milli> duration = now - startTime;

  const float dur = duration.count();
  const float estimation = (dur / progress) - dur;
  return estimation;
}
#endif // _ESTIMATE_H_