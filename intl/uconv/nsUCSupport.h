




#ifndef nsUCvJaSupport_h___
#define nsUCvJaSupport_h___

#include "nsCOMPtr.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "uconvutil.h"
#include "mozilla/Mutex.h"

#define ONE_BYTE_TABLE_SIZE 256

inline bool WillOverrun(char16_t* aDest, char16_t* aDestEnd, uint32_t aLength)
{
  NS_ASSERTION(aDest <= aDestEnd, "Pointer overrun even before check");
  return (uint32_t(aDestEnd - aDest) < aLength);
}
#define CHECK_OVERRUN(dest, destEnd, length) (WillOverrun(dest, destEnd, length))

#ifdef DEBUG

#define NS_IBASICDECODER_IID \
{ 0x7afc9f0a, 0xcfe1, 0x44ea, { 0xa7, 0x55, 0xe3, 0xb8, 0x6a, 0xb1, 0x22, 0x6e } }


#define NS_IBASICENCODER_IID \
{ 0x65968a7b, 0x6467, 0x4c4a, { 0xb5, 0xa, 0x3e, 0xc, 0x97, 0xa3, 0x2f, 0x7 } }

class nsIBasicDecoder : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IBASICDECODER_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIBasicDecoder, NS_IBASICDECODER_IID)

class nsIBasicEncoder : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IBASICENCODER_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIBasicEncoder, NS_IBASICENCODER_IID)

#endif



















class nsBasicDecoderSupport : public nsIUnicodeDecoder
#ifdef DEBUG
                              ,public nsIBasicDecoder
#endif
{
  NS_DECL_THREADSAFE_ISUPPORTS

public:

  


  nsBasicDecoderSupport();

  
  

  virtual void SetInputErrorBehavior(int32_t aBehavior);
  virtual char16_t GetCharacterForUnMapped();

protected:
  int32_t   mErrBehavior;

  


  virtual ~nsBasicDecoderSupport();
};













class nsBufferDecoderSupport : public nsBasicDecoderSupport
{
protected:

  


  char *    mBuffer;
  int32_t   mBufferCapacity;
  int32_t   mBufferLength;

  uint32_t  mMaxLengthFactor;
  
  


  NS_IMETHOD ConvertNoBuff(const char * aSrc, int32_t * aSrcLength, 
      char16_t * aDest, int32_t * aDestLength) = 0;

  void FillBuffer(const char ** aSrc, int32_t aSrcLength);

public:

  


  explicit nsBufferDecoderSupport(uint32_t aMaxLengthFactor);

  


  virtual ~nsBufferDecoderSupport();

  
  

  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength, 
      char16_t * aDest, int32_t * aDestLength);
  NS_IMETHOD Reset();
  NS_IMETHOD GetMaxLength(const char *aSrc,
                          int32_t aSrcLength,
                          int32_t* aDestLength);
};










class nsTableDecoderSupport : public nsBufferDecoderSupport
{
public:

  


  nsTableDecoderSupport(uScanClassID aScanClass, uShiftInTable * aShiftInTable,
      uMappingTable * aMappingTable, uint32_t aMaxLengthFactor);

  


  virtual ~nsTableDecoderSupport();

protected:

  uScanClassID              mScanClass;
  uShiftInTable             * mShiftInTable;
  uMappingTable             * mMappingTable;

  
  

  NS_IMETHOD ConvertNoBuff(const char * aSrc, int32_t * aSrcLength, 
      char16_t * aDest, int32_t * aDestLength);
};










class nsMultiTableDecoderSupport : public nsBufferDecoderSupport
{
public:

  


  nsMultiTableDecoderSupport(int32_t aTableCount, const uRange * aRangeArray, 
                             uScanClassID * aScanClassArray,
                             uMappingTable ** aMappingTable,
                             uint32_t aMaxLengthFactor);

  


  virtual ~nsMultiTableDecoderSupport();

protected:

  int32_t                   mTableCount;
  const uRange              * mRangeArray;
  uScanClassID              * mScanClassArray;
  uMappingTable             ** mMappingTable;

  
  

  NS_IMETHOD ConvertNoBuff(const char * aSrc, int32_t * aSrcLength, 
      char16_t * aDest, int32_t * aDestLength);
};










class nsOneByteDecoderSupport : public nsBasicDecoderSupport
{
public:

  


  explicit nsOneByteDecoderSupport(uMappingTable * aMappingTable);

  


  virtual ~nsOneByteDecoderSupport();

protected:

  uMappingTable             * mMappingTable;
  char16_t                 mFastTable[ONE_BYTE_TABLE_SIZE];
  bool                      mFastTableCreated;
  mozilla::Mutex            mFastTableMutex;

  
  

  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength, 
      char16_t * aDest, int32_t * aDestLength);
  NS_IMETHOD GetMaxLength(const char * aSrc, int32_t aSrcLength, 
      int32_t * aDestLength);
  NS_IMETHOD Reset();
};




class nsBasicEncoder : public nsIUnicodeEncoder
#ifdef DEBUG
                       ,public nsIBasicEncoder
#endif
{
  NS_DECL_ISUPPORTS

public:
  


  nsBasicEncoder();

protected:
  


  virtual ~nsBasicEncoder();

};














class nsEncoderSupport :  public nsBasicEncoder
{

protected:

  


  char *    mBuffer;
  int32_t   mBufferCapacity;
  char *    mBufferStart;
  char *    mBufferEnd;

  


  int32_t   mErrBehavior;
  nsCOMPtr<nsIUnicharEncoder> mErrEncoder;
  char16_t mErrChar;
  uint32_t  mMaxLengthFactor;

  



  NS_IMETHOD ConvertNoBuff(const char16_t * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength);

  



  NS_IMETHOD ConvertNoBuffNoErr(const char16_t * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength) = 0;

  


  NS_IMETHOD FinishNoBuff(char * aDest, int32_t * aDestLength);

  


  nsresult FlushBuffer(char ** aDest, const char * aDestEnd);

public:

  


  explicit nsEncoderSupport(uint32_t aMaxLengthFactor);

  


  virtual ~nsEncoderSupport();

  
  

  NS_IMETHOD Convert(const char16_t * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength);
  NS_IMETHOD Finish(char * aDest, int32_t * aDestLength);
  NS_IMETHOD Reset();
  NS_IMETHOD SetOutputErrorBehavior(int32_t aBehavior, 
      nsIUnicharEncoder * aEncoder, char16_t aChar);
  NS_IMETHOD GetMaxLength(const char16_t * aSrc, 
                          int32_t aSrcLength, 
                          int32_t * aDestLength);
};










class nsTableEncoderSupport : public nsEncoderSupport
{
public:

  


  nsTableEncoderSupport(uScanClassID  aScanClass,
                        uShiftOutTable * aShiftOutTable,
                        uMappingTable  * aMappingTable,
                        uint32_t aMaxLengthFactor);

  nsTableEncoderSupport(uScanClassID  aScanClass,
                        uMappingTable  * aMappingTable,
                        uint32_t aMaxLengthFactor);

  


  virtual ~nsTableEncoderSupport();

protected:

  uScanClassID              mScanClass;
  uShiftOutTable            * mShiftOutTable;
  uMappingTable             * mMappingTable;

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const char16_t * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength);
};










class nsMultiTableEncoderSupport : public nsEncoderSupport
{
public:

  


  nsMultiTableEncoderSupport(int32_t aTableCount,
                             uScanClassID * aScanClassArray,
                             uShiftOutTable ** aShiftOutTable,
                             uMappingTable  ** aMappingTable,
                             uint32_t aMaxLengthFactor);

  


  virtual ~nsMultiTableEncoderSupport();

protected:

  int32_t                   mTableCount;
  uScanClassID              * mScanClassArray;
  uShiftOutTable            ** mShiftOutTable;
  uMappingTable             ** mMappingTable;

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const char16_t * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength);
};

                        
#endif
