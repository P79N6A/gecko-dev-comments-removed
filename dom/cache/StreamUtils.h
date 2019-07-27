





#ifndef mozilla_dom_cache_StreamUtils_h
#define mozilla_dom_cache_StreamUtils_h

#include "nsTArrayForwardDeclare.h"

namespace mozilla {
namespace dom {
namespace cache {

class Feature;
class PCacheRequest;
class PCacheResponse;
class PCacheResponseOrVoid;

void StartDestroyStreamChild(const PCacheResponseOrVoid& aResponseOrVoid);
void StartDestroyStreamChild(const PCacheResponse& aResponse);
void StartDestroyStreamChild(const nsTArray<PCacheResponse>& aResponses);
void StartDestroyStreamChild(const nsTArray<PCacheRequest>& aRequests);

void AddFeatureToStreamChild(const PCacheResponseOrVoid& aResponseOrVoid,
                             Feature* aFeature);
void AddFeatureToStreamChild(const PCacheResponse& aResponse,
                             Feature* aFeature);
void AddFeatureToStreamChild(const nsTArray<PCacheResponse>& aResponses,
                              Feature* aFeature);
void AddFeatureToStreamChild(const nsTArray<PCacheRequest>& aRequests,
                              Feature* aFeature);

} 
} 
} 

#endif 
