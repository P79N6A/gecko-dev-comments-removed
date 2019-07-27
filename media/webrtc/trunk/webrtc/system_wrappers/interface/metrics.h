









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_METRICS_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_METRICS_H_

#include <string>

#include "webrtc/common_types.h"























































#define RTC_HISTOGRAM_COUNTS_100(name, sample) RTC_HISTOGRAM_COUNTS( \
    name, sample, 1, 100, 50)

#define RTC_HISTOGRAM_COUNTS_1000(name, sample) RTC_HISTOGRAM_COUNTS( \
    name, sample, 1, 1000, 50)

#define RTC_HISTOGRAM_COUNTS_10000(name, sample) RTC_HISTOGRAM_COUNTS( \
    name, sample, 1, 10000, 50)

#define RTC_HISTOGRAM_COUNTS(name, sample, min, max, bucket_count) \
    RTC_HISTOGRAM_COMMON_BLOCK(name, sample, \
        webrtc::metrics::HistogramFactoryGetCounts( \
            name, min, max, bucket_count))


#define RTC_HISTOGRAM_PERCENTAGE(name, sample) \
    RTC_HISTOGRAM_ENUMERATION(name, sample, 101)



#define RTC_HISTOGRAM_ENUMERATION(name, sample, boundary) \
    RTC_HISTOGRAM_COMMON_BLOCK(name, sample, \
        webrtc::metrics::HistogramFactoryGetEnumeration(name, boundary))

#define RTC_HISTOGRAM_COMMON_BLOCK(constant_name, sample, \
                                   factory_get_invocation) \
  do { \
    webrtc::metrics::Histogram* histogram_pointer = factory_get_invocation; \
    webrtc::metrics::HistogramAdd(histogram_pointer, constant_name, sample); \
  } while (0)


namespace webrtc {
namespace metrics {

class Histogram;





Histogram* HistogramFactoryGetCounts(
    const std::string& name, int min, int max, int bucket_count);



Histogram* HistogramFactoryGetEnumeration(
    const std::string& name, int boundary);



void HistogramAdd(
    Histogram* histogram_pointer, const std::string& name, int sample);

}  
}  

#endif  

