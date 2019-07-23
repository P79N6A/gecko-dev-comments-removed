






































#ifndef nsIFrameDebug_h___
#define nsIFrameDebug_h___

#include "nsISupports.h"
#include "nsIFrame.h"

class nsPresContext;
struct PRLogModuleInfo;


#define NS_IFRAMEDEBUG_IID         \
{ 0xa6cf9069, 0x15b3, 0x11d2, \
  {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}
 



class nsIFrameDebug : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAMEDEBUG_IID)
  
  NS_IMETHOD  List(FILE* out, PRInt32 aIndent) const = 0;
  



  static void RootFrameList(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent);

  static void DumpFrameTree(nsIFrame* aFrame);

  



  NS_IMETHOD  GetFrameName(nsAString& aResult) const = 0;
  


  NS_IMETHOD_(nsFrameState)  GetDebugStateBits() const = 0;
  









  NS_IMETHOD  DumpRegressionData(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent, PRBool aIncludeStyleData) = 0;

  NS_IMETHOD  VerifyTree() const = 0;

  





  static PRBool GetVerifyTreeEnable();

  


  static void SetVerifyTreeEnable(PRBool aEnabled);

  





  static PRBool GetVerifyStyleTreeEnable();

  


  static void SetVerifyStyleTreeEnable(PRBool aEnabled);

  






  static PRLogModuleInfo* GetLogModuleInfo();

  
  static void ShowFrameBorders(PRBool aEnable);
  static PRBool GetShowFrameBorders();

  
  static void ShowEventTargetFrameBorder(PRBool aEnable);
  static PRBool GetShowEventTargetFrameBorder();
  
  static void PrintDisplayList(nsDisplayListBuilder* aBuilder, const nsDisplayList& aList);

private:
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFrameDebug, NS_IFRAMEDEBUG_IID)

#endif 
