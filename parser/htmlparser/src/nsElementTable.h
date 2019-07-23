













































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

typedef PRBool (*ContainFunc)(eHTMLTags aTag,nsDTDContext &aContext);









inline PRBool TestBits(int aBitset,int aTest) {
  if(aTest) {
    PRInt32 result=(aBitset & aTest);
    return PRBool(result==aTest);
  }
  return PR_FALSE;
}








struct nsHTMLElement {

#ifdef DEBUG
  static  void    DebugDumpMembership(const char* aFilename);
  static  void    DebugDumpContainment(const char* aFilename,const char* aTitle);
  static  void    DebugDumpContainType(const char* aFilename);
#endif

  static  PRBool  IsInlineEntity(eHTMLTags aTag);
  static  PRBool  IsFlowEntity(eHTMLTags aTag);
  static  PRBool  IsBlockCloser(eHTMLTags aTag);

  inline  PRBool  IsBlock(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kBlock);
                    } 
                    return PR_FALSE;
                  }

  inline  PRBool  IsBlockEntity(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kBlockEntity);
                    } 
                    return PR_FALSE;
                  }

  inline  PRBool  IsSpecialEntity(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kSpecial);
                    } 
                    return PR_FALSE;
                  }

  inline  PRBool  IsPhraseEntity(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kPhrase);
                    } 
                    return PR_FALSE;
                  }

  inline  PRBool  IsFontStyleEntity(void) const { 
                    if((mTagID>=eHTMLTag_unknown) & (mTagID<=eHTMLTag_xmp)){
                      return TestBits(mParentBits,kFontStyle);
                    } 
                    return PR_FALSE;
                  }
  
  inline  PRBool  IsTableElement(void) const {  
                    PRBool result=PR_FALSE;

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
                        result=PR_TRUE;
                        break;
                      default:
                        result=PR_FALSE;
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

  PRBool          IsMemberOf(PRInt32 aType) const;
  PRBool          ContainsSet(PRInt32 aType) const;
  PRBool          CanContainType(PRInt32 aType) const;
  
  eHTMLTags       GetTag(void) const {return mTagID;}
  PRBool          CanContain(eHTMLTags aChild,nsDTDMode aMode) const;
  PRBool          CanExclude(eHTMLTags aChild) const;
  PRBool          CanOmitStartTag(eHTMLTags aChild) const;
  PRBool          CanOmitEndTag(void) const;
  PRBool          CanContainSelf(void) const;
  PRBool          CanAutoCloseTag(nsDTDContext& aContext,PRInt32 aIndex,eHTMLTags aTag) const;
  PRBool          HasSpecialProperty(PRInt32 aProperty) const;
  PRBool          IsSpecialParent(eHTMLTags aTag) const;
  PRBool          IsExcludableParent(eHTMLTags aParent) const;
  PRBool          SectionContains(eHTMLTags aTag,PRBool allowDepthSearch) const;
  PRBool          ShouldVerifyHierarchy() const;

  PRBool          CanBeContained(eHTMLTags aParentTag,nsDTDContext &aContext) const; 

  static  PRBool  CanContain(eHTMLTags aParent,eHTMLTags aChild,nsDTDMode aMode);
  static  PRBool  IsContainer(eHTMLTags aTag) ;
  static  PRBool  IsResidualStyleTag(eHTMLTags aTag) ;
  static  PRBool  IsTextTag(eHTMLTags aTag);
  static  PRBool  IsWhitespaceTag(eHTMLTags aTag);

  static  PRBool  IsBlockParent(eHTMLTags aTag);
  static  PRBool  IsInlineParent(eHTMLTags aTag); 
  static  PRBool  IsFlowParent(eHTMLTags aTag);
  static  PRBool  IsSectionTag(eHTMLTags aTag);
  static  PRBool  IsChildOfHead(eHTMLTags aTag,PRBool& aExclusively) ;

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
  eHTMLTags       mSkipTarget;        
  ContainFunc     mCanBeContained;
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
