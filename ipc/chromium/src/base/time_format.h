






#ifndef BASE_TIME_FORMAT_H_
#define BASE_TIME_FORMAT_H_

#include <string>

namespace base {

class Time;


std::wstring TimeFormatTimeOfDay(const Time& time);


std::wstring TimeFormatShortDate(const Time& time);


std::wstring TimeFormatShortDateNumeric(const Time& time);



std::wstring TimeFormatShortDateAndTime(const Time& time);



std::wstring TimeFormatFriendlyDateAndTime(const Time& time);



std::wstring TimeFormatFriendlyDate(const Time& time);

}  

#endif  
