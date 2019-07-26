




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
    std::string mBreakpadId;

    bool operator==(const Module& other) const;
  };

  const Frame &GetFrame(unsigned aIndex) const;
  void AddFrame(const Frame& aFrame);
  const Module &GetModule(unsigned aIndex) const;
  void AddModule(const Module& aFrame);

  void Clear();

private:
  std::vector<Module> mModules;
  std::vector<Frame> mStack;
};




ProcessedStack
GetStackAndModules(const std::vector<uintptr_t> &aPCs);

} 
} 
#endif 
