








#ifndef INDEXCHARS_H
#define INDEXCHARS_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/locid.h"


#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_NORMALIZATION






U_CDECL_BEGIN







typedef enum UAlphabeticIndexLabelType {
    




    U_ALPHAINDEX_NORMAL    = 0,

    




    U_ALPHAINDEX_UNDERFLOW = 1,

    







    U_ALPHAINDEX_INFLOW    = 2,

    




    U_ALPHAINDEX_OVERFLOW  = 3
} UAlphabeticIndexLabelType;


struct UHashtable;
U_CDECL_END

U_NAMESPACE_BEGIN



class Collator;
class RuleBasedCollator;
class StringEnumeration;
class UnicodeSet;
class UVector;






















































































class U_I18N_API AlphabeticIndex: public UObject {

  public:

    











     AlphabeticIndex(const Locale &locale, UErrorCode &status);



    









     virtual AlphabeticIndex &addLabels(const UnicodeSet &additions, UErrorCode &status);

    












     virtual AlphabeticIndex &addLabels(const Locale &locale, UErrorCode &status);

     



     virtual ~AlphabeticIndex();


    











    virtual const RuleBasedCollator &getCollator() const;


   







    virtual const UnicodeString &getInflowLabel() const;

   










    virtual AlphabeticIndex &setInflowLabel(const UnicodeString &inflowLabel, UErrorCode &status);



   






    virtual const UnicodeString &getOverflowLabel() const;


   








    virtual AlphabeticIndex &setOverflowLabel(const UnicodeString &overflowLabel, UErrorCode &status);

   






    virtual const UnicodeString &getUnderflowLabel() const;

   








    virtual AlphabeticIndex &setUnderflowLabel(const UnicodeString &underflowLabel, UErrorCode &status);


    






    virtual int32_t getMaxLabelCount() const;

    











    virtual AlphabeticIndex &setMaxLabelCount(int32_t maxLabelCount, UErrorCode &status);


    











    virtual const UnicodeString &getOverflowComparisonString(const UnicodeString &lowerLimit,
                                                             UErrorCode &status);


    















    virtual AlphabeticIndex &addRecord(const UnicodeString &name, const void *data, UErrorCode &status);

    







    virtual AlphabeticIndex &clearRecords(UErrorCode &status);


    







    virtual int32_t  getBucketCount(UErrorCode &status);


    







    virtual int32_t  getRecordCount(UErrorCode &status);



    











    virtual int32_t  getBucketIndex(const UnicodeString &itemName, UErrorCode &status);


    





    virtual int32_t  getBucketIndex() const;


    










    virtual UBool nextBucket(UErrorCode &status);

    







    virtual const UnicodeString &getBucketLabel() const;

    






    virtual UAlphabeticIndexLabelType getBucketLabelType() const;

    







    virtual int32_t getBucketRecordCount() const;


    







    virtual AlphabeticIndex &resetBucketIterator(UErrorCode &status);

    










    virtual UBool nextRecord(UErrorCode &status);

    







    virtual const UnicodeString &getRecordName() const;


    







    virtual const void *getRecordData() const;


    





    virtual AlphabeticIndex &resetRecordIterator();

private:
    
    virtual UClassID getDynamicClassID() const;

     



     AlphabeticIndex(const AlphabeticIndex &other);

     


     AlphabeticIndex &operator =(const AlphabeticIndex & ) { return *this;};

    



     virtual UBool operator==(const AlphabeticIndex& other) const;

    



     virtual UBool operator!=(const AlphabeticIndex& other) const;

     
     void init(UErrorCode &status);

     
     static void staticInit(UErrorCode &status);

     
     void hackName(UnicodeString &dest, const UnicodeString &name, const Collator *coll);
     void initPinyinBounds(const Collator *coll, UErrorCode &status);

   public:
#ifndef U_HIDE_INTERNAL_API
     




     static void staticCleanup();
#endif  
   private:

     
     
     static void getIndexExemplars(UnicodeSet &dest, const Locale &locale, UErrorCode &status);

     UVector *firstStringsInScript(UErrorCode &status);

     static UnicodeString separated(const UnicodeString &item);

     static UnicodeSet *getScriptSet(UnicodeSet &dest, const UnicodeString &codePoint, UErrorCode &status);

     void buildIndex(UErrorCode &status);
     void buildBucketList(UErrorCode &status);
     void bucketRecords(UErrorCode &status);


  public:

    
    
    

#ifndef U_HIDE_INTERNAL_API
    



     struct Record: public UMemory {
         AlphabeticIndex     *alphaIndex_;
         const UnicodeString  name_;
         UnicodeString        sortingName_;  
         const void           *data_;
         int32_t              serialNumber_;  
         Record(AlphabeticIndex *alphaIndex, const UnicodeString &name, const void *data);
         ~Record();
     };
#endif  

     




     UVector  *inputRecords_;

     




     struct Bucket: public UMemory {
         UnicodeString     label_;
         UnicodeString     lowerBoundary_;
         UAlphabeticIndexLabelType labelType_;
         UVector           *records_; 

         Bucket(const UnicodeString &label,   
                const UnicodeString &lowerBoundary,
                UAlphabeticIndexLabelType type, UErrorCode &status);
         ~Bucket();
     };

  public:

    



    enum ELangType {
        
        kNormal,
        
        kSimplified,
        
        kTraditional
    };

    



    static ELangType  langTypeFromLocale(const Locale &loc);


   private:

     
     
     UVector *bucketList_;

     int32_t  labelsIterIndex_;      
     int32_t  itemsIterIndex_;
     Bucket   *currentBucket_;       
                                     
                                     

     UBool    indexBuildRequired_;   
                                     
                                     

     int32_t    maxLabelCount_;      

     UHashtable *alreadyIn_;         

     UnicodeSet *initialLabels_;     
                                     
                                     
                                     

     UVector    *labels_;            
                                     

     UnicodeSet *noDistinctSorting_; 
                                     
                                     
                                     

     UnicodeSet *notAlphabetic_;     
                                     
                                     
                                     


     UVector    *firstScriptCharacters_;  
                                          

     Locale    locale_;
     Collator  *collator_;
     Collator  *collatorPrimaryOnly_;

     UnicodeString  inflowLabel_;
     UnicodeString  overflowLabel_;
     UnicodeString  underflowLabel_;
     UnicodeString  overflowComparisonString_;

     ELangType      langType_;        
                                      

     typedef const UChar PinyinLookup[24][3];
     static PinyinLookup   HACK_PINYIN_LOOKUP_SHORT;
     static PinyinLookup   HACK_PINYIN_LOOKUP_LONG;
     
     
     
     static PinyinLookup   *HACK_PINYIN_LOOKUP;
     static const UChar    *PINYIN_LOWER_BOUNDS;



     int32_t    recordCounter_;         



     static UnicodeSet *ALPHABETIC;
     static UnicodeSet *CORE_LATIN;
     static UnicodeSet *ETHIOPIC;
     static UnicodeSet *HANGUL;
     static UnicodeSet *IGNORE_SCRIPTS;
     static UnicodeSet *TO_TRY;
     static UnicodeSet *UNIHAN;
     static const UnicodeString *EMPTY_STRING;

};

U_NAMESPACE_END

#endif
#endif
