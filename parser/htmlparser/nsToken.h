




#ifndef CTOKEN__
#define CTOKEN__

enum eHTMLTokenTypes {
  eToken_unknown=0,
  eToken_start=1,      eToken_end,          eToken_comment,         eToken_entity,
  eToken_whitespace,   eToken_newline,      eToken_text,            eToken_attribute,
  eToken_instruction,  eToken_cdatasection, eToken_doctypeDecl,     eToken_markupDecl,
  eToken_last 
};

#endif


