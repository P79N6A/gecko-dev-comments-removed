




































#ifndef nsUCvJaSupport_h___
#define nsUCvJaSupport_h___

#include "nsCOMPtr.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"
#include "nsICharRepresentable.h"
#include "uconvutil.h"
#include "mozilla/Mutex.h"

#define ONE_BYTE_TABLE_SIZE 256

#ifdef NS_DEBUG

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
#ifdef NS_DEBUG
                              ,public nsIBasicDecoder
#endif
{
  NS_DECL_ISUPPORTS

public:

  


  nsBasicDecoderSupport();

  


  virtual ~nsBasicDecoderSupport();

  
  

  virtual void SetInputErrorBehavior(PRInt32 aBehavior);
  virtual PRUnichar GetCharacterForUnMapped();

protected:
  PRInt32   mErrBehavior;
};













class nsBufferDecoderSupport : public nsBasicDecoderSupport
{
protected:

  


  char *    mBuffer;
  PRInt32   mBufferCapacity;
  PRInt32   mBufferLength;

  PRUint32  mMaxLengthFactor;
  
  


  NS_IMETHOD ConvertNoBuff(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength) = 0;

  void FillBuffer(const char ** aSrc, PRInt32 aSrcLength);
  void DoubleBuffer();

public:

  


  nsBufferDecoderSupport(PRUint32 aMaxLengthFactor);

  


  virtual ~nsBufferDecoderSupport();

  
  

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Reset();
  NS_IMETHOD GetMaxLength(const char *aSrc,
                          PRInt32 aSrcLength,
                          PRInt32* aDestLength);
};










class nsTableDecoderSupport : public nsBufferDecoderSupport
{
public:

  


  nsTableDecoderSupport(uScanClassID aScanClass, uShiftInTable * aShiftInTable,
      uMappingTable * aMappingTable, PRUint32 aMaxLengthFactor);

  


  virtual ~nsTableDecoderSupport();

protected:

  uScanClassID              mScanClass;
  uShiftInTable             * mShiftInTable;
  uMappingTable             * mMappingTable;

  
  

  NS_IMETHOD ConvertNoBuff(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
};










class nsMultiTableDecoderSupport : public nsBufferDecoderSupport
{
public:

  


  nsMultiTableDecoderSupport(PRInt32 aTableCount, const uRange * aRangeArray, 
                             uScanClassID * aScanClassArray,
                             uMappingTable ** aMappingTable,
                             PRUint32 aMaxLengthFactor);

  


  virtual ~nsMultiTableDecoderSupport();

protected:

  PRInt32                   mTableCount;
  const uRange              * mRangeArray;
  uScanClassID              * mScanClassArray;
  uMappingTable             ** mMappingTable;

  
  

  NS_IMETHOD ConvertNoBuff(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
};










class nsOneByteDecoderSupport : public nsBasicDecoderSupport
{
public:

  


  nsOneByteDecoderSupport(uMappingTable * aMappingTable);

  


  virtual ~nsOneByteDecoderSupport();

protected:

  uMappingTable             * mMappingTable;
  PRUnichar                 mFastTable[ONE_BYTE_TABLE_SIZE];
  PRBool                    mFastTableCreated;
  mozilla::Mutex            mFastTableMutex;

  
  

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength);
  NS_IMETHOD Reset();
};




class nsBasicEncoder : public nsIUnicodeEncoder, public nsICharRepresentable
#ifdef NS_DEBUG
                       ,public nsIBasicEncoder
#endif
{
  NS_DECL_ISUPPORTS

public:
  


  nsBasicEncoder();

  


  virtual ~nsBasicEncoder();

};














class nsEncoderSupport :  public nsBasicEncoder
{

protected:

  


  char *    mBuffer;
  PRInt32   mBufferCapacity;
  char *    mBufferStart;
  char *    mBufferEnd;

  


  PRInt32   mErrBehavior;
  nsCOMPtr<nsIUnicharEncoder> mErrEncoder;
  PRUnichar mErrChar;
  PRUint32  mMaxLengthFactor;

  



  NS_IMETHOD ConvertNoBuff(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);

  



  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength) = 0;

  


  NS_IMETHOD FinishNoBuff(char * aDest, PRInt32 * aDestLength);

  


  nsresult FlushBuffer(char ** aDest, const char * aDestEnd);

public:

  


  nsEncoderSupport(PRUint32 aMaxLengthFactor);

  


  virtual ~nsEncoderSupport();

  
  

  NS_IMETHOD Convert(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Reset();
  NS_IMETHOD SetOutputErrorBehavior(PRInt32 aBehavior, 
      nsIUnicharEncoder * aEncoder, PRUnichar aChar);
  NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, 
                          PRInt32 aSrcLength, 
                          PRInt32 * aDestLength);

  
  
  NS_IMETHOD FillInfo(PRUint32 *aInfo) = 0;
};










class nsTableEncoderSupport : public nsEncoderSupport
{
public:

  


  nsTableEncoderSupport(uScanClassID  aScanClass,
                        uShiftOutTable * aShiftOutTable,
                        uMappingTable  * aMappingTable,
                        PRUint32 aMaxLengthFactor);

  nsTableEncoderSupport(uScanClassID  aScanClass,
                        uMappingTable  * aMappingTable,
                        PRUint32 aMaxLengthFactor);

  


  virtual ~nsTableEncoderSupport();
  NS_IMETHOD FillInfo( PRUint32 *aInfo);

protected:

  uScanClassID              mScanClass;
  uShiftOutTable            * mShiftOutTable;
  uMappingTable             * mMappingTable;

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
};










class nsMultiTableEncoderSupport : public nsEncoderSupport
{
public:

  


  nsMultiTableEncoderSupport(PRInt32 aTableCount,
                             uScanClassID * aScanClassArray,
                             uShiftOutTable ** aShiftOutTable,
                             uMappingTable  ** aMappingTable,
                             PRUint32 aMaxLengthFactor);

  


  virtual ~nsMultiTableEncoderSupport();
  NS_IMETHOD FillInfo( PRUint32 *aInfo);

protected:

  PRInt32                   mTableCount;
  uScanClassID              * mScanClassArray;
  uShiftOutTable            ** mShiftOutTable;
  uMappingTable             ** mMappingTable;

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
};

                        
#endif
