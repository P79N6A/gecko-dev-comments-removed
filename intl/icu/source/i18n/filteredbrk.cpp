






#include "unicode/utypes.h"
#if !UCONFIG_NO_BREAK_ITERATION && !UCONFIG_NO_FILTERED_BREAK_ITERATION

#include "cmemory.h"

#include "unicode/filteredbrk.h"
#include "unicode/ucharstriebuilder.h"
#include "unicode/ures.h"

#include "uresimp.h" 
#include "ubrkimpl.h" 
#include "uvector.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

#ifndef FB_DEBUG
#define FB_DEBUG 0
#endif

#if FB_DEBUG
#include <stdio.h>
static void _fb_trace(const char *m, const UnicodeString *s, UBool b, int32_t d, const char *f, int l) {
  char buf[2048];
  if(s) {
    s->extract(0,s->length(),buf,2048);
  } else {
    strcpy(buf,"NULL");
  }
  fprintf(stderr,"%s:%d: %s. s='%s'(%p), b=%c, d=%d\n",
          f, l, m, buf, (const void*)s, b?'T':'F',(int)d);
}

#define FB_TRACE(m,s,b,d) _fb_trace(m,s,b,d,__FILE__,__LINE__)
#else
#define FB_TRACE(m,s,b,d)
#endif

static int8_t U_CALLCONV compareUnicodeString(UElement t1, UElement t2) {
    const UnicodeString &a = *(const UnicodeString*)t1.pointer;
    const UnicodeString &b = *(const UnicodeString*)t2.pointer;
    return a.compare(b);
}




class U_I18N_API UStringSet : public UVector {
 public:
  UStringSet(UErrorCode &status) : UVector(uprv_deleteUObject,
                                           uhash_compareUnicodeString,
                                           1,
                                           status) {}
  virtual ~UStringSet();
  


  inline UBool contains(const UnicodeString& s) {
    return contains((void*) &s);
  }
  using UVector::contains;
  


  inline const UnicodeString* getStringAt(int32_t i) const {
    return (const UnicodeString*)elementAt(i);
  }
  




  inline UBool adopt(UnicodeString *str, UErrorCode &status) {
    if(U_FAILURE(status) || contains(*str)) {
      delete str;
      return false;
    } else {
      sortedInsert(str, compareUnicodeString, status);
      if(U_FAILURE(status)) {
        delete str;
        return false;
      }
      return true;
    }
  }
  



  inline UBool add(const UnicodeString& str, UErrorCode &status) {
    if(U_FAILURE(status)) return false;
    UnicodeString *t = new UnicodeString(str);
    if(t==NULL) {
      status = U_MEMORY_ALLOCATION_ERROR; return false;
    }
    return adopt(t, status);
  }
  



  inline UBool remove(const UnicodeString &s, UErrorCode &status) {
    if(U_FAILURE(status)) return false;
    return removeElement((void*) &s);
  }
};




UStringSet::~UStringSet() {}


static const int32_t kPARTIAL = (1<<0); 
static const int32_t kMATCH   = (1<<1); 
static const int32_t kSuppressInReverse = (1<<0);
static const int32_t kAddToForward = (1<<1);
static const UChar kFULLSTOP = 0x002E; 

class SimpleFilteredSentenceBreakIterator : public BreakIterator {
public:
  SimpleFilteredSentenceBreakIterator(BreakIterator *adopt, UCharsTrie *forwards, UCharsTrie *backwards, UErrorCode &status);
  SimpleFilteredSentenceBreakIterator(const SimpleFilteredSentenceBreakIterator& other);
  virtual ~SimpleFilteredSentenceBreakIterator();
private:
  LocalPointer<BreakIterator> fDelegate;
  LocalUTextPointer           fText;
  LocalPointer<UCharsTrie>    fBackwardsTrie; 
  LocalPointer<UCharsTrie>    fForwardsPartialTrie; 

  
public:
  
  virtual BreakIterator *  createBufferClone(void * ,
                                             int32_t &,
                                             UErrorCode &status) {
    
    status = U_SAFECLONE_ALLOCATED_WARNING;
    return clone();
  }
  virtual BreakIterator* clone(void) const { return new SimpleFilteredSentenceBreakIterator(*this); }
  virtual UClassID getDynamicClassID(void) const { return NULL; }
  virtual UBool operator==(const BreakIterator& o) const { if(this==&o) return true; return false; }

  
  virtual void setText(UText *text, UErrorCode &status) { fDelegate->setText(text,status); }
  virtual BreakIterator &refreshInputText(UText *input, UErrorCode &status) { fDelegate->refreshInputText(input,status); return *this; }
  virtual void adoptText(CharacterIterator* it) { fDelegate->adoptText(it); }
  virtual void setText(const UnicodeString &text) { fDelegate->setText(text); }

  
  virtual UText *getUText(UText *fillIn, UErrorCode &status) const { return fDelegate->getUText(fillIn,status); }
  virtual CharacterIterator& getText(void) const { return fDelegate->getText(); }

  
  virtual int32_t first(void) { return fDelegate->first(); }
  virtual int32_t preceding(int32_t ) {  return UBRK_DONE; }
  virtual int32_t previous(void) {  return UBRK_DONE; }
  virtual UBool isBoundary(int32_t offset) { return fDelegate->isBoundary(offset); }
  virtual int32_t current(void) const { return fDelegate->current(); }

  virtual int32_t next(void);

  virtual int32_t next(int32_t ) {  return UBRK_DONE; }
  virtual int32_t following(int32_t ) {  return UBRK_DONE; }
  virtual int32_t last(void) { return fDelegate->last(); }

};

SimpleFilteredSentenceBreakIterator::SimpleFilteredSentenceBreakIterator(const SimpleFilteredSentenceBreakIterator& other)
  : BreakIterator(other), fDelegate(other.fDelegate->clone())
{
  








}


SimpleFilteredSentenceBreakIterator::SimpleFilteredSentenceBreakIterator(BreakIterator *adopt, UCharsTrie *forwards, UCharsTrie *backwards, UErrorCode &status) :
  BreakIterator(adopt->getLocale(ULOC_VALID_LOCALE,status),adopt->getLocale(ULOC_ACTUAL_LOCALE,status)),
  fDelegate(adopt),
  fBackwardsTrie(backwards),
  fForwardsPartialTrie(forwards)
{
  
}

SimpleFilteredSentenceBreakIterator::~SimpleFilteredSentenceBreakIterator() {}

int32_t SimpleFilteredSentenceBreakIterator::next() {
  int32_t n = fDelegate->next();
  if(n == UBRK_DONE || 
     fBackwardsTrie.isNull()) { 
    return n;
  }
  
  UErrorCode status = U_ZERO_ERROR;
  
  fText.adoptInstead(fDelegate->getUText(fText.orphan(), status));
  
  do { 
    
    utext_setNativeIndex(fText.getAlias(), n); 
    fBackwardsTrie->reset();
    UChar32 uch;
    
    
    if((uch=utext_previous32(fText.getAlias()))==(UChar32)0x0020) {  
      
      
    } else {
      
      uch = utext_next32(fText.getAlias());
      
    }
    UStringTrieResult r = USTRINGTRIE_INTERMEDIATE_VALUE;

    int32_t bestPosn = -1;
    int32_t bestValue = -1;

    while((uch=utext_previous32(fText.getAlias()))!=U_SENTINEL  &&   
          USTRINGTRIE_HAS_NEXT(r=fBackwardsTrie->nextForCodePoint(uch))) {
      if(USTRINGTRIE_HAS_VALUE(r)) { 
        bestPosn = utext_getNativeIndex(fText.getAlias());
        bestValue = fBackwardsTrie->getValue();
      }
      
    }

    if(USTRINGTRIE_MATCHES(r)) { 
      
      bestValue = fBackwardsTrie->getValue();
      bestPosn = utext_getNativeIndex(fText.getAlias());
      
    }

    if(bestPosn>=0) {
      

      
      
      

      if(bestValue == kMATCH) { 
        
        n = fDelegate->next(); 
        if(n==UBRK_DONE) return n;
        continue; 
      } else if(bestValue == kPARTIAL
                && fForwardsPartialTrie.isValid()) { 
        
        
        
        fForwardsPartialTrie->reset();
        UStringTrieResult rfwd = USTRINGTRIE_INTERMEDIATE_VALUE;
        utext_setNativeIndex(fText.getAlias(), bestPosn); 
        
        while((uch=utext_next32(fText.getAlias()))!=U_SENTINEL &&
              USTRINGTRIE_HAS_NEXT(rfwd=fForwardsPartialTrie->nextForCodePoint(uch))) {
          
        }
        if(USTRINGTRIE_MATCHES(rfwd)) {
          
          
          
          n = fDelegate->next();
          if(n==UBRK_DONE) return n;
          continue;
        } else {
          
          
          return n;
        }
      } else {
        return n; 
      }
    } else {
      
      return n; 
    }
  } while(n != UBRK_DONE);
  return n;
}




class U_I18N_API SimpleFilteredBreakIteratorBuilder : public FilteredBreakIteratorBuilder {
public:
  virtual ~SimpleFilteredBreakIteratorBuilder();
  SimpleFilteredBreakIteratorBuilder(const Locale &fromLocale, UErrorCode &status);
  SimpleFilteredBreakIteratorBuilder(UErrorCode &status);
  virtual UBool suppressBreakAfter(const UnicodeString& exception, UErrorCode& status);
  virtual UBool unsuppressBreakAfter(const UnicodeString& exception, UErrorCode& status);
  virtual BreakIterator *build(BreakIterator* adoptBreakIterator, UErrorCode& status);
private:
  UStringSet fSet;
};

SimpleFilteredBreakIteratorBuilder::~SimpleFilteredBreakIteratorBuilder()
{
}

SimpleFilteredBreakIteratorBuilder::SimpleFilteredBreakIteratorBuilder(UErrorCode &status) 
  : fSet(status)
{
}

SimpleFilteredBreakIteratorBuilder::SimpleFilteredBreakIteratorBuilder(const Locale &fromLocale, UErrorCode &status)
  : fSet(status)
{
  if(U_SUCCESS(status)) {
    LocalUResourceBundlePointer b(ures_open(U_ICUDATA_BRKITR, fromLocale.getBaseName(), &status));
    LocalUResourceBundlePointer exceptions(ures_getByKeyWithFallback(b.getAlias(), "exceptions", NULL, &status));
    LocalUResourceBundlePointer breaks(ures_getByKeyWithFallback(exceptions.getAlias(), "SentenceBreak", NULL, &status));
    if(U_FAILURE(status)) return; 

    LocalUResourceBundlePointer strs;
    UErrorCode subStatus = status;
    do {
      strs.adoptInstead(ures_getNextResource(breaks.getAlias(), strs.orphan(), &subStatus));
      if(strs.isValid() && U_SUCCESS(subStatus)) {
        UnicodeString str(ures_getUnicodeString(strs.getAlias(), &status));
        suppressBreakAfter(str, status); 
      }
    } while (strs.isValid() && U_SUCCESS(subStatus));
    if(U_FAILURE(subStatus)&&subStatus!=U_INDEX_OUTOFBOUNDS_ERROR&&U_SUCCESS(status)) {
      status = subStatus;
    }
  }
}

UBool
SimpleFilteredBreakIteratorBuilder::suppressBreakAfter(const UnicodeString& exception, UErrorCode& status)
{
  UBool r = fSet.add(exception, status);
  FB_TRACE("suppressBreakAfter",&exception,r,0);
  return r;
}

UBool
SimpleFilteredBreakIteratorBuilder::unsuppressBreakAfter(const UnicodeString& exception, UErrorCode& status)
{
  UBool r = fSet.remove(exception, status);
  FB_TRACE("unsuppressBreakAfter",&exception,r,0);
  return r;
}








static inline UnicodeString* newUnicodeStringArray(size_t count) {
    return new UnicodeString[count ? count : 1];
}

BreakIterator *
SimpleFilteredBreakIteratorBuilder::build(BreakIterator* adoptBreakIterator, UErrorCode& status) {
  LocalPointer<BreakIterator> adopt(adoptBreakIterator);

  LocalPointer<UCharsTrieBuilder> builder(new UCharsTrieBuilder(status), status);
  LocalPointer<UCharsTrieBuilder> builder2(new UCharsTrieBuilder(status), status);
  if(U_FAILURE(status)) {
    return NULL;
  }

  int32_t revCount = 0;
  int32_t fwdCount = 0;

  int32_t subCount = fSet.size();

  UnicodeString *ustrs_ptr = newUnicodeStringArray(subCount);
  
  LocalArray<UnicodeString> ustrs(ustrs_ptr);

  LocalMemory<int> partials;
  partials.allocateInsteadAndReset(subCount);

  LocalPointer<UCharsTrie>    backwardsTrie; 
  LocalPointer<UCharsTrie>    forwardsPartialTrie; 

  int n=0;
  for ( int32_t i = 0;
        i<fSet.size();
        i++) {
    const UnicodeString *abbr = fSet.getStringAt(i);
    if(abbr) {
      FB_TRACE("build",abbr,TRUE,i);
      ustrs[n] = *abbr; 
      FB_TRACE("ustrs[n]",&ustrs[n],TRUE,i);
    } else {
      FB_TRACE("build",abbr,FALSE,i);
      status = U_MEMORY_ALLOCATION_ERROR;
      return NULL;
    }
    partials[n] = 0; 
    n++;
  }
  
  for(int i=0;i<subCount;i++) {
    int nn = ustrs[i].indexOf(kFULLSTOP); 
    if(nn>-1 && (nn+1)!=ustrs[i].length()) {
      FB_TRACE("partial",&ustrs[i],FALSE,i);
      
      
      int sameAs = -1;
      for(int j=0;j<subCount;j++) {
        if(j==i) continue;
        if(ustrs[i].compare(0,nn+1,ustrs[j],0,nn+1)==0) {
          FB_TRACE("prefix",&ustrs[j],FALSE,nn+1);
          
          if(partials[j]==0) { 
            partials[j] = kSuppressInReverse | kAddToForward;
            FB_TRACE("suppressing",&ustrs[j],FALSE,j);
          } else if(partials[j] & kSuppressInReverse) {
            sameAs = j; 
          }
        }
      }
      FB_TRACE("for partial same-",&ustrs[i],FALSE,sameAs);
      FB_TRACE(" == partial #",&ustrs[i],FALSE,partials[i]);
      UnicodeString prefix(ustrs[i], 0, nn+1);
      if(sameAs == -1 && partials[i] == 0) {
        
        prefix.reverse();
        builder->add(prefix, kPARTIAL, status);
        revCount++;
        FB_TRACE("Added partial",&prefix,FALSE, i);
        FB_TRACE(u_errorName(status),&ustrs[i],FALSE,i);
        partials[i] = kSuppressInReverse | kAddToForward;
      } else {
        FB_TRACE("NOT adding partial",&prefix,FALSE, i);
        FB_TRACE(u_errorName(status),&ustrs[i],FALSE,i);
      }
    }
  }
  for(int i=0;i<subCount;i++) {
    if(partials[i]==0) {
      ustrs[i].reverse();
      builder->add(ustrs[i], kMATCH, status);
      revCount++;
      FB_TRACE(u_errorName(status), &ustrs[i], FALSE, i);
    } else {
      FB_TRACE("Adding fwd",&ustrs[i], FALSE, i);

      
      
      
      
      builder2->add(ustrs[i], kMATCH, status); 
      fwdCount++;
      
      
    }
  }
  FB_TRACE("AbbrCount",NULL,FALSE, subCount);

  if(revCount>0) {
    backwardsTrie.adoptInstead(builder->build(USTRINGTRIE_BUILD_FAST, status));
    if(U_FAILURE(status)) {
      FB_TRACE(u_errorName(status),NULL,FALSE, -1);
      return NULL;
    }
  }

  if(fwdCount>0) {
    forwardsPartialTrie.adoptInstead(builder2->build(USTRINGTRIE_BUILD_FAST, status));
    if(U_FAILURE(status)) {
      FB_TRACE(u_errorName(status),NULL,FALSE, -1);
      return NULL;
    }
  }

  return new SimpleFilteredSentenceBreakIterator(adopt.orphan(), forwardsPartialTrie.orphan(), backwardsTrie.orphan(), status);
}




FilteredBreakIteratorBuilder::FilteredBreakIteratorBuilder() {
}

FilteredBreakIteratorBuilder::~FilteredBreakIteratorBuilder() {
}

FilteredBreakIteratorBuilder *
FilteredBreakIteratorBuilder::createInstance(const Locale& where, UErrorCode& status) {
  if(U_FAILURE(status)) return NULL;
  LocalPointer<FilteredBreakIteratorBuilder> ret(new SimpleFilteredBreakIteratorBuilder(where, status), status);
  return ret.orphan();
}

FilteredBreakIteratorBuilder *
FilteredBreakIteratorBuilder::createInstance(UErrorCode& status) {
  if(U_FAILURE(status)) return NULL;
  LocalPointer<FilteredBreakIteratorBuilder> ret(new SimpleFilteredBreakIteratorBuilder(status), status);
  return ret.orphan();
}

U_NAMESPACE_END

#endif 
