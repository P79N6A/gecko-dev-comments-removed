








#ifndef INDEXCHARS_H
#define INDEXCHARS_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/locid.h"

#if !UCONFIG_NO_COLLATION






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



class BucketList;
class Collator;
class RuleBasedCollator;
class StringEnumeration;
class UnicodeSet;
class UVector;











































































































class U_I18N_API AlphabeticIndex: public UObject {
public:
     







     class U_I18N_API Bucket : public UObject {
     public:
        



        virtual ~Bucket();

        





        const UnicodeString &getLabel() const { return label_; }
        





        UAlphabeticIndexLabelType getLabelType() const { return labelType_; }

     private:
        friend class AlphabeticIndex;
        friend class BucketList;

        UnicodeString label_;
        UnicodeString lowerBoundary_;
        UAlphabeticIndexLabelType labelType_;
        Bucket *displayBucket_;
        int32_t displayIndex_;
        UVector *records_;  

        Bucket(const UnicodeString &label,   
               const UnicodeString &lowerBoundary,
               UAlphabeticIndexLabelType type);
     };

    









    class U_I18N_API ImmutableIndex : public UObject {
    public:
        



        virtual ~ImmutableIndex();

        





        int32_t getBucketCount() const;

        







        int32_t getBucketIndex(const UnicodeString &name, UErrorCode &errorCode) const;

        






        const Bucket *getBucket(int32_t index) const;

    private:
        friend class AlphabeticIndex;

        ImmutableIndex(BucketList *bucketList, Collator *collatorPrimaryOnly)
                : buckets_(bucketList), collatorPrimaryOnly_(collatorPrimaryOnly) {}

        BucketList *buckets_;
        Collator *collatorPrimaryOnly_;
    };

    











     AlphabeticIndex(const Locale &locale, UErrorCode &status);

   













    AlphabeticIndex(RuleBasedCollator *collator, UErrorCode &status);

    









    virtual AlphabeticIndex &addLabels(const UnicodeSet &additions, UErrorCode &status);

    












    virtual AlphabeticIndex &addLabels(const Locale &locale, UErrorCode &status);

     



    virtual ~AlphabeticIndex();

    





    ImmutableIndex *buildImmutableIndex(UErrorCode &errorCode);

    











    virtual const RuleBasedCollator &getCollator() const;


   







    virtual const UnicodeString &getInflowLabel() const;

   










    virtual AlphabeticIndex &setInflowLabel(const UnicodeString &inflowLabel, UErrorCode &status);


   






    virtual const UnicodeString &getOverflowLabel() const;


   








    virtual AlphabeticIndex &setOverflowLabel(const UnicodeString &overflowLabel, UErrorCode &status);

   






    virtual const UnicodeString &getUnderflowLabel() const;

   








    virtual AlphabeticIndex &setUnderflowLabel(const UnicodeString &underflowLabel, UErrorCode &status);


    






    virtual int32_t getMaxLabelCount() const;

    











    virtual AlphabeticIndex &setMaxLabelCount(int32_t maxLabelCount, UErrorCode &status);


    















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
     



     AlphabeticIndex(const AlphabeticIndex &other);

     


     AlphabeticIndex &operator =(const AlphabeticIndex & ) { return *this;};

    



     virtual UBool operator==(const AlphabeticIndex& other) const;

    



     virtual UBool operator!=(const AlphabeticIndex& other) const;

     
     void init(const Locale *locale, UErrorCode &status);

    



    void addIndexExemplars(const Locale &locale, UErrorCode &status);
    


    UBool addChineseIndexCharacters(UErrorCode &errorCode);

    UVector *firstStringsInScript(UErrorCode &status);

    static UnicodeString separated(const UnicodeString &item);

    




    void initLabels(UVector &indexCharacters, UErrorCode &errorCode) const;
    BucketList *createBucketList(UErrorCode &errorCode) const;
    void initBuckets(UErrorCode &errorCode);
    void clearBuckets();
    void internalResetBucketIterator();

public:

    
    
    

#ifndef U_HIDE_INTERNAL_API
    




    struct Record: public UMemory {
        const UnicodeString  name_;
        const void           *data_;
        Record(const UnicodeString &name, const void *data);
        ~Record();
    };
#endif  

private:

    




    UVector  *inputList_;

    int32_t  labelsIterIndex_;        
    int32_t  itemsIterIndex_;
    Bucket   *currentBucket_;         
                                      
                                      

    int32_t    maxLabelCount_;        

    UnicodeSet *initialLabels_;       
                                      
                                      
                                      

    UVector *firstCharsInScripts_;    
                                      

    RuleBasedCollator *collator_;
    RuleBasedCollator *collatorPrimaryOnly_;

    
    BucketList *buckets_;

    UnicodeString  inflowLabel_;
    UnicodeString  overflowLabel_;
    UnicodeString  underflowLabel_;
    UnicodeString  overflowComparisonString_;

    UnicodeString emptyString_;
};

U_NAMESPACE_END

#endif  
#endif
