





#ifndef mozilla_AnimationComparator_h
#define mozilla_AnimationComparator_h

namespace mozilla {





template<typename AnimationPtrType>
class AnimationPtrComparator {
public:
  bool Equals(const AnimationPtrType& a, const AnimationPtrType& b) const
  {
    return a == b;
  }

  bool LessThan(const AnimationPtrType& a, const AnimationPtrType& b) const
  {
    return a->HasLowerCompositeOrderThan(*b);
  }
};

} 

#endif 
