




































#ifndef nsHtml5StringParser_h_
#define nsHtml5StringParser_h_

#include "nsHtml5AtomTable.h"
#include "nsParserBase.h"

class nsHtml5TreeOpExecutor;
class nsHtml5TreeBuilder;
class nsHtml5Tokenizer;
class nsIContent;

class nsHtml5StringParser : public nsParserBase
{
  public:

    NS_DECL_ISUPPORTS

    nsHtml5StringParser();
    virtual ~nsHtml5StringParser();

    











    nsresult ParseHtml5Fragment(const nsAString& aSourceBuffer,
                                nsIContent* aTargetNode,
                                nsIAtom* aContextLocalName,
                                PRInt32 aContextNamespace,
                                bool aQuirks,
                                bool aPreventScriptExecution);

  private:

    


    nsRefPtr<nsHtml5TreeOpExecutor>     mExecutor;

    


    const nsAutoPtr<nsHtml5TreeBuilder> mTreeBuilder;

    


    const nsAutoPtr<nsHtml5Tokenizer>   mTokenizer;

    


    nsHtml5AtomTable                    mAtomTable;

};

#endif 
