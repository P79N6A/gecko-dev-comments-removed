













































#ifndef _NSELEMENTABLE
#define _NSELEMENTABLE

#include "nsHTMLTokens.h"
#include "nsDTDUtils.h"






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







#ifdef NS_DEBUG
extern void CheckElementTable();
#endif









inline bool TestBits(int aBitset,int aTest) {
  if(aTest) {
    PRInt32 result=(aBitset & aTest);
    return bool(result==aTest);
  }
  return false;
}








struct nsHTMLElement {

#ifdef DEBUG
  static  void    DebugDumpMembership(const char* aFilename);
  static  void    DebugDumpContainment(const char* aFilename,const char* aTitle);
  static  void    DebugDumpContainType(const char* aFilename);
#endif

  static  bool    IsInlineEntity(eHTMLTags aTag);
  static  bool    IsFlowEntity(eHTMLTags aTag);
  static  bool    IsBlockCloser(eHTMLTags aTag);

  inline  bool    IsBlock(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kBlock);
                    } 
                    return false;
                  }

  inline  bool    IsBlockEntity(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kBlockEntity);
                    } 
                    return false;
                  }

  inline  bool    IsSpecialEntity(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kSpecial);
                    } 
                    return false;
                  }

  inline  bool    IsPhraseEntity(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kPhrase);
                    } 
                    return false;
                  }

  inline  bool    IsFontStyleEntity(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kFontStyle);
                    } 
                    return false;
                  }
  
  inline  bool    IsTableElement(void) const {  
                    bool result=false;

                    switch(mTagID) {
                      case eHTMLTag_table:
                      case eHTMLTag_thead:
                      case eHTMLTag_tbody:
                      case eHTMLTag_tfoot:
                      case eHTMLTag_caption:
                      case eHTMLTag_tr:
                      case eHTMLTag_td:
                      case eHTMLTag_th:
                      case eHTMLTag_col:
                      case eHTMLTag_colgroup:
                        result=true;
                        break;
                      default:
                        result=false;
                    }
                    return result;
                  }


  static  PRInt32 GetIndexOfChildOrSynonym(nsDTDContext& aContext,eHTMLTags aChildTag);

  const TagList*  GetSynonymousTags(void) const {return mSynonymousTags;}
  const TagList*  GetRootTags(void) const {return mRootNodes;}
  const TagList*  GetEndRootTags(void) const {return mEndRootNodes;}
  const TagList*  GetAutoCloseStartTags(void) const {return mAutocloseStart;}
  const TagList*  GetAutoCloseEndTags(void) const {return mAutocloseEnd;}
  eHTMLTags       GetCloseTargetForEndTag(nsDTDContext& aContext,PRInt32 anIndex,nsDTDMode aMode) const;

  const TagList*        GetSpecialChildren(void) const {return mSpecialKids;}
  const TagList*        GetSpecialParents(void) const {return mSpecialParents;}

  bool            IsMemberOf(PRInt32 aType) const;
  bool            ContainsSet(PRInt32 aType) const;
  bool            CanContainType(PRInt32 aType) const;

  bool            CanContain(eHTMLTags aChild,nsDTDMode aMode) const;
  bool            CanExclude(eHTMLTags aChild) const;
  bool            CanOmitEndTag(void) const;
  bool            CanContainSelf(void) const;
  bool            CanAutoCloseTag(nsDTDContext& aContext,PRInt32 aIndex,eHTMLTags aTag) const;
  bool            HasSpecialProperty(PRInt32 aProperty) const;
  bool            IsSpecialParent(eHTMLTags aTag) const;
  bool            IsExcludableParent(eHTMLTags aParent) const;
  bool            SectionContains(eHTMLTags aTag,bool allowDepthSearch) const;
  bool            ShouldVerifyHierarchy() const;

  static  bool    CanContain(eHTMLTags aParent,eHTMLTags aChild,nsDTDMode aMode);
  static  bool    IsContainer(eHTMLTags aTag) ;
  static  bool    IsResidualStyleTag(eHTMLTags aTag) ;
  static  bool    IsTextTag(eHTMLTags aTag);
  static  bool    IsWhitespaceTag(eHTMLTags aTag);

  static  bool    IsBlockParent(eHTMLTags aTag);
  static  bool    IsInlineParent(eHTMLTags aTag); 
  static  bool    IsFlowParent(eHTMLTags aTag);
  static  bool    IsSectionTag(eHTMLTags aTag);
  static  bool    IsChildOfHead(eHTMLTags aTag,bool& aExclusively) ;

  eHTMLTags       mTagID;
  eHTMLTags       mRequiredAncestor;
  eHTMLTags       mExcludingAncestor; 
  const TagList*  mRootNodes;         
  const TagList*        mEndRootNodes;      
  const TagList*        mAutocloseStart;    
  const TagList*        mAutocloseEnd;      
  const TagList*        mSynonymousTags;    
  const TagList*        mExcludableParents; 
  int             mParentBits;        
  int             mInclusionBits;     
  int             mExclusionBits;     
  int             mSpecialProperties; 
  PRUint32        mPropagateRange;    
  const TagList*  mSpecialParents;    
  const TagList*  mSpecialKids;       
}; 

extern const nsHTMLElement gHTMLElements[];


static const int kPreferBody       = 0x0001; 
static const int kOmitEndTag       = 0x0002; 
static const int kLegalOpen        = 0x0004; 
static const int kNoPropagate      = 0x0008; 
static const int kBadContentWatch  = 0x0010; 

static const int kNoStyleLeaksIn   = 0x0020; 
static const int kNoStyleLeaksOut  = 0x0040; 

static const int kMustCloseSelf    = 0x0080; 
static const int kSaveMisplaced    = 0x0100; 
static const int kNonContainer     = 0x0200; 
static const int kHandleStrayTag   = 0x0400; 
static const int kRequiresBody     = 0x0800; 
static const int kVerifyHierarchy  = 0x1000; 
static const int kPreferHead       = 0x2000; 

#endif
