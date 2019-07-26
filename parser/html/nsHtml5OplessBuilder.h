





#ifndef nsHtml5OplessBuilder_h
#define nsHtml5OplessBuilder_h

#include "nsHtml5DocumentBuilder.h"

class nsParserBase;

class nsHtml5OplessBuilder : public nsHtml5DocumentBuilder
{
public:
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  nsHtml5OplessBuilder();
  ~nsHtml5OplessBuilder();
  void Start();
  void Finish();
  void SetParser(nsParserBase* aParser);
};

#endif 
