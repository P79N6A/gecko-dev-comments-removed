













#ifndef _NSELEMENTABLE
#define _NSELEMENTABLE

#include "nsHTMLTags.h"
#include "nsIDTD.h"





static const int kNone= 0x0;

static const int kHTMLContent   = 0x0001; 
static const int kHeadContent   = 0x0002; 
static const int kHeadMisc      = 0x0004; 

static const int kSpecial       = 0x0008; 
                                          

static const int kFormControl   = 0x0010; 
static const int kPreformatted  = 0x0020; 
static const int kPreExclusion  = 0x0040; 
static const int kFontStyle     = 0x0080; 
static const int kPhrase        = 0x0100; 
static const int kHeading       = 0x0200; 
static const int kBlockMisc     = 0x0400; 
static const int kBlock         = 0x0800; 
                                          
static const int kList          = 0x1000; 
static const int kPCDATA        = 0x2000; 
static const int kSelf          = 0x4000; 
static const int kExtensions    = 0x8000; 
static const int kTable         = 0x10000;
static const int kDLChild       = 0x20000;
static const int kCDATA         = 0x40000;

static const int kInlineEntity  = (kPCDATA|kFontStyle|kPhrase|kSpecial|kFormControl|kExtensions);  
static const int kBlockEntity   = (kHeading|kList|kPreformatted|kBlock); 
static const int kFlowEntity    = (kBlockEntity|kInlineEntity); 
static const int kAllTags       = 0xffffff;






#ifdef DEBUG
extern void CheckElementTable();
#endif








inline bool TestBits(int aBitset,int aTest) {
  if(aTest) {
    int32_t result=(aBitset & aTest);
    return bool(result==aTest);
  }
  return false;
}

struct nsHTMLElement {
  bool            IsMemberOf(int32_t aType) const;

  eHTMLTags       mTagID;
  int             mParentBits;        
  bool            mLeaf;

  static  bool    IsContainer(eHTMLTags aTag);
}; 

extern const nsHTMLElement gHTMLElements[];

#endif
