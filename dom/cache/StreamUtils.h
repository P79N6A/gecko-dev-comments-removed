





#ifndef mozilla_dom_cache_StreamUtils_h
#define mozilla_dom_cache_StreamUtils_h

#include "nsTArrayForwardDeclare.h"

namespace mozilla {
namespace dom {
namespace cache {

class Feature;
class CacheRequest;
class CacheResponse;
class CacheResponseOrVoid;

void StartDestroyStreamChild(const CacheResponseOrVoid& aResponseOrVoid);
void StartDestroyStreamChild(const CacheResponse& aResponse);
void StartDestroyStreamChild(const nsTArray<CacheResponse>& aResponses);
void StartDestroyStreamChild(const nsTArray<CacheRequest>& aRequests);

void AddFeatureToStreamChild(const CacheResponseOrVoid& aResponseOrVoid,
                             Feature* aFeature);
void AddFeatureToStreamChild(const CacheResponse& aResponse,
                             Feature* aFeature);
void AddFeatureToStreamChild(const nsTArray<CacheResponse>& aResponses,
                              Feature* aFeature);
void AddFeatureToStreamChild(const nsTArray<CacheRequest>& aRequests,
                              Feature* aFeature);

} 
} 
} 

#endif 
