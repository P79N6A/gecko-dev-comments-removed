




































#ifndef nsAHtml5TreeBuilderState_h___
#define nsAHtml5TreeBuilderState_h___






class nsAHtml5TreeBuilderState {
  public:
  
    virtual jArray<nsHtml5StackNode*,PRInt32> getStack() = 0;
    
    virtual jArray<nsHtml5StackNode*,PRInt32> getListOfActiveFormattingElements() = 0;
    
    virtual PRInt32 getStackLength() = 0;

    virtual PRInt32 getListOfActiveFormattingElementsLength() = 0;

    virtual nsIContent** getFormPointer() = 0;
    
    virtual nsIContent** getHeadPointer() = 0;

    virtual nsIContent** getDeepTreeSurrogateParent() = 0;

    virtual PRInt32 getMode() = 0;

    virtual PRInt32 getOriginalMode() = 0;

    virtual bool isFramesetOk() = 0;

    virtual bool isNeedToDropLF() = 0;

    virtual bool isQuirks() = 0;
    
    virtual ~nsAHtml5TreeBuilderState() {
    }
};

#endif 
