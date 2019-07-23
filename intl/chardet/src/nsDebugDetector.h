



































#ifndef nsDebugDetector_h__
#define nsDebugDetector_h__

#include "nsIFactory.h"
#include "nsICharsetDetector.h"



#define NS_1STBLKDBG_DETECTOR_CID \
{ 0x12bb8f18, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_2NDBLKDBG_DETECTOR_CID \
{ 0x12bb8f19, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_LASTBLKDBG_DETECTOR_CID \
{ 0x12bb8f1a, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }



typedef enum {
  k1stBlk,
  k2ndBlk,
  klastBlk
} nsDebugDetectorSel;

class nsDebugDetector : public nsICharsetDetector 
{
  NS_DECL_ISUPPORTS

public:  
  nsDebugDetector(nsDebugDetectorSel aSel);
  virtual ~nsDebugDetector();

  NS_IMETHOD Init(nsICharsetDetectionObserver* aObserver);

  NS_IMETHOD DoIt(const char* aBytesArray, PRUint32 aLen, PRBool* oDontFeedMe);

  NS_IMETHOD Done();

protected:

  virtual void Report();

private:
  PRInt32 mBlks;
  nsDebugDetectorSel mSel;
  nsICharsetDetectionObserver* mObserver;
  PRBool mStop;
};

class ns1stBlkDbgDetector : public nsDebugDetector
{
public:
    ns1stBlkDbgDetector () : nsDebugDetector(k1stBlk) {} ;
};

class ns2ndBlkDbgDetector : public nsDebugDetector
{
public:
  ns2ndBlkDbgDetector () : nsDebugDetector(k2ndBlk) {} ;
};

class nsLastBlkDbgDetector : public nsDebugDetector
{
public:
  nsLastBlkDbgDetector () : nsDebugDetector(klastBlk) {} ;
};

#endif 
