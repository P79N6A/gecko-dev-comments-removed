








































#ifndef __nsIRollupListener_h__
#define __nsIRollupListener_h__

class nsIContent;

class nsIRollupListener {
 public: 

  






  NS_IMETHOD Rollup(PRUint32 aCount, nsIContent **aContent) = 0;

  


  NS_IMETHOD ShouldRollupOnMouseWheelEvent(PRBool *aShould) = 0;

  


  NS_IMETHOD ShouldRollupOnMouseActivate(PRBool *aShould) = 0;

};

#endif 
