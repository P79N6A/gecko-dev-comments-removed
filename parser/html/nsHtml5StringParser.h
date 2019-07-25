



#ifndef nsHtml5StringParser_h_
#define nsHtml5StringParser_h_

#include "nsHtml5AtomTable.h"
#include "nsParserBase.h"

class nsHtml5TreeOpExecutor;
class nsHtml5TreeBuilder;
class nsHtml5Tokenizer;
class nsIContent;
class nsIDocument;

class nsHtml5StringParser : public nsParserBase
{
  public:

    NS_DECL_ISUPPORTS

    



    nsHtml5StringParser();
    virtual ~nsHtml5StringParser();

    












    nsresult ParseFragment(const nsAString& aSourceBuffer,
                           nsIContent* aTargetNode,
                           nsIAtom* aContextLocalName,
                           int32_t aContextNamespace,
                           bool aQuirks,
                           bool aPreventScriptExecution);

    




    nsresult ParseDocument(const nsAString& aSourceBuffer,
                           nsIDocument* aTargetDoc,
                           bool aScriptingEnabledForNoscriptParsing);

  private:

    void Tokenize(const nsAString& aSourceBuffer,
                  nsIDocument* aDocument,
                  bool aScriptingEnabledForNoscriptParsing);

    


    nsRefPtr<nsHtml5TreeOpExecutor>     mExecutor;

    


    const nsAutoPtr<nsHtml5TreeBuilder> mTreeBuilder;

    


    const nsAutoPtr<nsHtml5Tokenizer>   mTokenizer;

    


    nsHtml5AtomTable                    mAtomTable;

};

#endif 
