









#ifndef TIMED_TRACE_H
#define TIMED_TRACE_H

#include "typedefs.h"

#include <stdio.h>
#include <stdlib.h>

class TimedTrace {
 public:
  TimedTrace();
  ~TimedTrace();

  void SetTimeEllapsed(double myTime);
  double TimeEllapsed();
  void Tick10Msec();
  int16_t SetUp(char* fileName);
  void TimedLogg(char* message);

 private:
  static double _timeEllapsedSec;
  static FILE* _timedTraceFile;

};

#endif
