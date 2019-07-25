



































#ifndef nsHtml5Highlighter_h_
#define nsHtml5Highlighter_h_

#include "prtypes.h"
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

    






    PRInt32 Transition(PRInt32 aState, bool aReconsume, PRInt32 aPos);

    


    void End();

    


    void SetBuffer(nsHtml5UTF16Buffer* aBuffer);

    




    void DropBuffer(PRInt32 aPos);

    




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

    





    void StartSpan(const PRUnichar* aClass);

    


    void EndSpanOrA();

    


    void StartCharacters();

    


    void EndCharacters();

    


    void StartA();

    


    void FlushChars();

    


    void FlushCurrent();

    





    void FinishTag();

    





    void AddClass(const PRUnichar* aClass);

    







    nsIContent** AllocateContentHandle();

    







    nsIContent** CreateElement(nsIAtom* aName,
                               nsHtml5HtmlAttributes* aAttributes);

    





    nsIContent** CurrentNode();

    






    void Push(nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes);

    


    void Pop();

    






    void AppendCharacters(const PRUnichar* aBuffer,
                          PRInt32 aStart,
                          PRInt32 aLength);

    





    void AddViewSourceHref(const nsString& aValue);

    


    PRInt32 mState;

    



    PRInt32 mCStart;

    



    PRInt32 mPos;

    


    PRInt32 mLineNumber;

    



    PRInt32 mUnicharsInThisPre;

    



    PRInt32 mInlinesOpen;

    



    bool mInCharacters;

    


    nsHtml5UTF16Buffer* mBuffer;

    


    bool mSyntaxHighlight;

    


    nsTArray<nsHtml5TreeOperation> mOpQueue;

    


    nsAHtml5TreeOpSink* mOpSink;

    


    nsIContent** mCurrentRun;

    



    nsIContent** mAmpersand;

    


    nsIContent** mSlash;

    


    nsAutoArrayPtr<nsIContent*> mHandles;

    


    PRInt32 mHandlesUsed;

    


    nsTArray<nsAutoArrayPtr<nsIContent*> > mOldHandles;

    


    nsTArray<nsIContent**> mStack;

    


    static PRUnichar sComment[];

    


    static PRUnichar sCdata[];

    


    static PRUnichar sStartTag[];

    


    static PRUnichar sAttributeName[];

    


    static PRUnichar sAttributeValue[];

    


    static PRUnichar sEndTag[];

    


    static PRUnichar sDoctype[];

    


    static PRUnichar sEntity[];

    


    static PRUnichar sPi[];
};

#endif 
