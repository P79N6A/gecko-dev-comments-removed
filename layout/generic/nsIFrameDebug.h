






































#ifndef nsIFrameDebug_h___
#define nsIFrameDebug_h___

#include "nsIFrame.h"

class nsPresContext;
struct PRLogModuleInfo;




class nsIFrameDebug {
public:
  NS_DECLARE_FRAME_ACCESSOR(nsIFrameDebug)
  
  NS_IMETHOD  List(FILE* out, PRInt32 aIndent) const = 0;
  



  static void RootFrameList(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent);

  static void DumpFrameTree(nsIFrame* aFrame);

  



  NS_IMETHOD  GetFrameName(nsAString& aResult) const = 0;
  


  NS_IMETHOD_(nsFrameState)  GetDebugStateBits() const = 0;
  







  NS_IMETHOD  DumpRegressionData(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent) = 0;

  





  static PRBool GetVerifyStyleTreeEnable();

  


  static void SetVerifyStyleTreeEnable(PRBool aEnabled);

  






  static PRLogModuleInfo* GetLogModuleInfo();

  
  static void ShowFrameBorders(PRBool aEnable);
  static PRBool GetShowFrameBorders();

  
  static void ShowEventTargetFrameBorder(PRBool aEnable);
  static PRBool GetShowEventTargetFrameBorder();
  
  static void PrintDisplayList(nsDisplayListBuilder* aBuilder, const nsDisplayList& aList);
};

#endif 
