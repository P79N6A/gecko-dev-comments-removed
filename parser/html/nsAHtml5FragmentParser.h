




































#ifndef nsAHtml5FragmentParser_h_
#define nsAHtml5FragmentParser_h_

#include "nsIParser.h"






class nsAHtml5FragmentParser : public nsIParser {
  public:

    











    NS_IMETHOD ParseHtml5Fragment(const nsAString& aSourceBuffer,
                                  nsIContent* aTargetNode,
                                  nsIAtom* aContextLocalName,
                                  PRInt32 aContextNamespace,
                                  PRBool aQuirks,
                                  PRBool aPreventScriptExecution) = 0;

};

#endif 
