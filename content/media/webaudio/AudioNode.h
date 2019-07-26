





#ifndef AudioNode_h_
#define AudioNode_h_

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "AudioContext.h"

struct JSContext;

namespace mozilla {

class ErrorResult;

namespace dom {

class AudioNode : public nsISupports,
                  public nsWrapperCache,
                  public EnableWebAudioCheck
{
public:
  explicit AudioNode(AudioContext* aContext);
  virtual ~AudioNode() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AudioNode)

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  AudioContext* Context() const
  {
    return mContext;
  }

  void Connect(AudioNode& aDestination, uint32_t aOutput,
               uint32_t aInput, ErrorResult& aRv);

  void Disconnect(uint32_t aOutput, ErrorResult& aRv);

  uint32_t NumberOfInputs() const
  {
    return mInputs.Length();
  }
  uint32_t NumberOfOutputs() const
  {
    return mOutputs.Length();
  }

  
  
  virtual uint32_t MaxNumberOfInputs() const = 0;
  virtual uint32_t MaxNumberOfOutputs() const = 0;

  struct Output {
    enum { InvalidIndex = 0xffffffff };
    Output()
      : mInput(InvalidIndex)
    {
    }
    Output(AudioNode* aDestination, uint32_t aInput)
      : mDestination(aDestination),
        mInput(aInput)
    {
    }

    
    typedef void**** ConvertibleToBool;
    operator ConvertibleToBool() const {
      return ConvertibleToBool(mDestination && mInput != InvalidIndex);
    }

    
    AudioNode* get() const {
      return mDestination;
    }

    nsRefPtr<AudioNode> mDestination;
    
    
    const uint32_t mInput;
  };
  struct Input {
    enum { InvalidIndex = 0xffffffff };
    Input()
      : mOutput(InvalidIndex)
    {
    }
    Input(AudioNode* aSource, uint32_t aOutput)
      : mSource(aSource),
        mOutput(aOutput)
    {
    }

    
    typedef void**** ConvertibleToBool;
    operator ConvertibleToBool() const {
      return ConvertibleToBool(mSource && mOutput != InvalidIndex);
    }

    
    AudioNode* get() const {
      return mSource;
    }

    nsRefPtr<AudioNode> mSource;
    
    
    const uint32_t mOutput;
  };

private:
  nsRefPtr<AudioContext> mContext;
  nsTArray<Input> mInputs;
  nsTArray<Output> mOutputs;
};

}
}

#endif

