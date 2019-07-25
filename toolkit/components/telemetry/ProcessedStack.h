




#ifndef ProcessedStack_h__
#define ProcessedStack_h__

#include <string>
#include <vector>

namespace mozilla {
namespace Telemetry {




class ProcessedStack
{
public:
  ProcessedStack();
  size_t GetStackSize() const;
  size_t GetNumModules() const;

  struct Frame
  {
    
    uintptr_t mOffset;
    
    
    uint16_t mModIndex;
  };
  struct Module
  {
    
    std::string mName;

    
    
    uintptr_t mStart;

    
    
    size_t mMappingSize;
    
    int mPdbAge;
    std::string mPdbSignature;
    std::string mPdbName;

    bool operator==(const Module& other) const;
  };

  const Frame &GetFrame(unsigned aIndex) const;
  void AddFrame(const Frame& aFrame);
  const Module &GetModule(unsigned aIndex) const;
  void AddModule(const Module& aFrame);

  void Clear();

  
  bool HasModule(const Module &aModule) const;
  void RemoveModule(unsigned aIndex);

private:
  std::vector<Module> mModules;
  std::vector<Frame> mStack;
};






ProcessedStack
GetStackAndModules(const std::vector<uintptr_t> &aPCs, bool aRelative);

} 
} 
#endif 
