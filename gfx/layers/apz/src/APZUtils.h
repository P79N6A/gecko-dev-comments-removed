





#ifndef mozilla_layers_APZUtils_h
#define mozilla_layers_APZUtils_h

namespace mozilla {
namespace layers {

enum HitTestResult {
  HitNothing,
  HitLayer,
  HitDispatchToContentRegion,
};

enum CancelAnimationFlags : uint32_t {
  Default = 0,            
  ExcludeOverscroll = 1   
};

}
}

#endif 
