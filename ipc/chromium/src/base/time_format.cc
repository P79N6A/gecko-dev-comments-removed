



#include "base/time_format.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "base/time.h"
#include "unicode/datefmt.h"

using base::Time;

namespace {

std::wstring TimeFormat(const DateFormat* formatter,
                        const Time& time) {
  DCHECK(formatter);
  UnicodeString date_string;

  formatter->format(static_cast<UDate>(time.ToDoubleT() * 1000), date_string);
  std::wstring output;
  bool success = UTF16ToWide(date_string.getBuffer(), date_string.length(),
      &output);
  DCHECK(success);
  return output;
}

}

namespace base {

std::wstring TimeFormatTimeOfDay(const Time& time) {
  
  
  scoped_ptr<DateFormat> formatter(DateFormat::createTimeInstance(
      DateFormat::kShort));
  return TimeFormat(formatter.get(), time);
}

std::wstring TimeFormatShortDate(const Time& time) {
  scoped_ptr<DateFormat> formatter(DateFormat::createDateInstance(
      DateFormat::kMedium));
  return TimeFormat(formatter.get(), time);
}

std::wstring TimeFormatShortDateNumeric(const Time& time) {
  scoped_ptr<DateFormat> formatter(DateFormat::createDateInstance(
      DateFormat::kShort));
  return TimeFormat(formatter.get(), time);
}

std::wstring TimeFormatShortDateAndTime(const Time& time) {
   scoped_ptr<DateFormat> formatter(DateFormat::createDateTimeInstance(
      DateFormat::kShort));
  return TimeFormat(formatter.get(), time);
}

std::wstring TimeFormatFriendlyDateAndTime(const Time& time) {
   scoped_ptr<DateFormat> formatter(DateFormat::createDateTimeInstance(
      DateFormat::kFull));
  return TimeFormat(formatter.get(), time);
}

std::wstring TimeFormatFriendlyDate(const Time& time) {
  scoped_ptr<DateFormat> formatter(DateFormat::createDateInstance(
      DateFormat::kFull));
  return TimeFormat(formatter.get(), time);
}

}  
