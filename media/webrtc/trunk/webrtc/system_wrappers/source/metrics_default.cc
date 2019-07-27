








#include "webrtc/system_wrappers/interface/metrics.h"




namespace webrtc {
namespace metrics {

Histogram* HistogramFactoryGetCounts(const std::string& name, int min, int max,
    int bucket_count) { return NULL; }

Histogram* HistogramFactoryGetEnumeration(const std::string& name,
    int boundary) { return NULL; }

void HistogramAdd(
    Histogram* histogram_pointer, const std::string& name, int sample) {}

}  
}  

