



#ifndef nsQueryFrame_h
#define nsQueryFrame_h

#include "nscore.h"
#include "mozilla/Assertions.h"
#include "mozilla/TypeTraits.h"




#define NS_DECL_QUERYFRAME_TARGET(classname)                    \
  static const nsQueryFrame::FrameIID kFrameIID = nsQueryFrame::classname##_id; \
  typedef classname Has_NS_DECL_QUERYFRAME_TARGET;

#define NS_DECL_QUERYFRAME                                      \
  virtual void* QueryFrame(FrameIID id);

#define NS_QUERYFRAME_HEAD(class)                               \
  void* class::QueryFrame(FrameIID id) { switch (id) {

#define NS_QUERYFRAME_ENTRY(class)                              \
  case class::kFrameIID: {                                      \
    MOZ_STATIC_ASSERT((mozilla::IsSame<class, class::Has_NS_DECL_QUERYFRAME_TARGET>::value), \
                      #class " must declare itself as a queryframe target"); \
    return static_cast<class*>(this);                           \
  }

#define NS_QUERYFRAME_ENTRY_CONDITIONAL(class, condition)       \
  case class::kFrameIID:                                        \
  if (condition) {                                              \
    MOZ_STATIC_ASSERT((mozilla::IsSame<class, class::Has_NS_DECL_QUERYFRAME_TARGET>::value), \
                      #class " must declare itself as a queryframe target"); \
    return static_cast<class*>(this);                           \
  }                                                             \
  break;

#define NS_QUERYFRAME_TAIL_INHERITING(class)                    \
  default: break;                                               \
  }                                                             \
  return class::QueryFrame(id);                                 \
}

#define NS_QUERYFRAME_TAIL_INHERITANCE_ROOT                     \
  default: break;                                               \
  }                                                             \
  MOZ_ASSERT(id != GetFrameId(),                                \
    "A frame failed to QueryFrame to its *own type*. "          \
    "It may be missing NS_DECL_QUERYFRAME, or a "               \
    "NS_QUERYFRAME_ENTRY() line with its own type name");       \
  return nullptr;                                               \
}

class nsQueryFrame
{
public:
  enum FrameIID {
#define FRAME_ID(classname) classname##_id,
#include "nsFrameIdList.h"
#undef FRAME_ID

    
    
    
    
    
    
    NON_FRAME_MARKER = 0x20000000
  };

  virtual void* QueryFrame(FrameIID id) = 0;
};

class do_QueryFrame
{
public:
  do_QueryFrame(nsQueryFrame *s) : mRawPtr(s) { }

  template<class Dest>
  operator Dest*() {
    MOZ_STATIC_ASSERT((mozilla::IsSame<Dest, typename Dest::Has_NS_DECL_QUERYFRAME_TARGET>::value),
                      "Dest must declare itself as a queryframe target");
    if (!mRawPtr)
      return nullptr;

    return reinterpret_cast<Dest*>(mRawPtr->QueryFrame(Dest::kFrameIID));
  }

private:
  nsQueryFrame *mRawPtr;
};

#endif 
