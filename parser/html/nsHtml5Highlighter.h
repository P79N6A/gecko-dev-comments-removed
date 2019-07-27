


#ifndef nsHtml5Highlighter_h
#define nsHtml5Highlighter_h

#include "nsCOMPtr.h"
#include "nsHtml5TreeOperation.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsHtml5TreeOperation.h"
#include "nsAHtml5TreeOpSink.h"

#define NS_HTML5_HIGHLIGHTER_HANDLE_ARRAY_LENGTH 512





class nsHtml5Highlighter
{
  public:
    




    nsHtml5Highlighter(nsAHtml5TreeOpSink* aOpSink);

    


    ~nsHtml5Highlighter();

    


    void Start(const nsAutoString& aTitle);

    






    int32_t Transition(int32_t aState, bool aReconsume, int32_t aPos);

    


    void End();

    


    void SetBuffer(nsHtml5UTF16Buffer* aBuffer);

    




    void DropBuffer(int32_t aPos);

    




    bool FlushOps();

    







    void MaybeLinkifyAttributeValue(nsHtml5AttributeName* aName,
                                    nsString* aValue);

    



    void CompletedNamedCharacterReference();

    





    void AddErrorToCurrentNode(const char* aMsgId);

    






    void AddErrorToCurrentRun(const char* aMsgId);

    







    void AddErrorToCurrentRun(const char* aMsgId, nsIAtom* aName);

    








    void AddErrorToCurrentRun(const char* aMsgId,
                              nsIAtom* aName,
                              nsIAtom* aOther);

    





    void AddErrorToCurrentAmpersand(const char* aMsgId);

    





    void AddErrorToCurrentSlash(const char* aMsgId);

  private:

    


    void StartSpan();

    





    void StartSpan(const char16_t* aClass);

    


    void EndSpanOrA();

    


    void StartCharacters();

    


    void EndCharactersAndStartMarkupRun();

    


    void StartA();

    


    void FlushChars();

    


    void FlushCurrent();

    





    void FinishTag();

    





    void AddClass(const char16_t* aClass);

    







    nsIContent** AllocateContentHandle();

    







    nsIContent** CreateElement(nsIAtom* aName,
                               nsHtml5HtmlAttributes* aAttributes);

    





    nsIContent** CurrentNode();

    






    void Push(nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes);

    


    void Pop();

    






    void AppendCharacters(const char16_t* aBuffer,
                          int32_t aStart,
                          int32_t aLength);

    





    void AddViewSourceHref(const nsString& aValue);

    


    int32_t mState;

    



    int32_t mCStart;

    



    int32_t mPos;

    


    int32_t mLineNumber;

    



    int32_t mInlinesOpen;

    



    bool mInCharacters;

    


    nsHtml5UTF16Buffer* mBuffer;

    


    bool mSyntaxHighlight;

    


    nsTArray<nsHtml5TreeOperation> mOpQueue;

    


    nsAHtml5TreeOpSink* mOpSink;

    


    nsIContent** mCurrentRun;

    



    nsIContent** mAmpersand;

    


    nsIContent** mSlash;

    


    nsAutoArrayPtr<nsIContent*> mHandles;

    


    int32_t mHandlesUsed;

    


    nsTArray<nsAutoArrayPtr<nsIContent*> > mOldHandles;

    


    nsTArray<nsIContent**> mStack;

    


    static char16_t sComment[];

    


    static char16_t sCdata[];

    


    static char16_t sStartTag[];

    


    static char16_t sAttributeName[];

    


    static char16_t sAttributeValue[];

    


    static char16_t sEndTag[];

    


    static char16_t sDoctype[];

    


    static char16_t sEntity[];

    


    static char16_t sPi[];
};

#endif 
