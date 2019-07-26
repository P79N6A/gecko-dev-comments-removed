



#ifndef nsAHtml5TreeBuilderState_h
#define nsAHtml5TreeBuilderState_h

#include "nsIContentHandle.h"






class nsAHtml5TreeBuilderState {
  public:
  
    virtual jArray<nsHtml5StackNode*,int32_t> getStack() = 0;
    
    virtual jArray<nsHtml5StackNode*,int32_t> getListOfActiveFormattingElements() = 0;

    virtual jArray<int32_t,int32_t> getTemplateModeStack() = 0;

    virtual int32_t getStackLength() = 0;

    virtual int32_t getListOfActiveFormattingElementsLength() = 0;

    virtual int32_t getTemplateModeStackLength() = 0;

    virtual nsIContentHandle* getFormPointer() = 0;
    
    virtual nsIContentHandle* getHeadPointer() = 0;

    virtual nsIContentHandle* getDeepTreeSurrogateParent() = 0;

    virtual int32_t getMode() = 0;

    virtual int32_t getOriginalMode() = 0;

    virtual bool isFramesetOk() = 0;

    virtual bool isNeedToDropLF() = 0;

    virtual bool isQuirks() = 0;
    
    virtual ~nsAHtml5TreeBuilderState() {
    }
};

#endif 
