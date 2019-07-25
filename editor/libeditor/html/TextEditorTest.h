




#ifndef __TextEditorTest_h__
#define __TextEditorTest_h__

#include "nscore.h"
#include "prtypes.h"

class nsIEditor;
class nsIPlaintextEditor;
#ifdef DEBUG

#include "nsCOMPtr.h"

class TextEditorTest
{
public:

  void Run(nsIEditor *aEditor, PRInt32 *outNumTests, PRInt32 *outNumTestsFailed);
  TextEditorTest();
  ~TextEditorTest();

protected:

  
  nsresult InitDoc();

  nsresult RunUnitTest(PRInt32 *outNumTests, PRInt32 *outNumTestsFailed);

  nsresult TestInsertBreak();

  nsresult TestTextProperties();

  nsCOMPtr<nsIPlaintextEditor> mTextEditor;
  nsCOMPtr<nsIEditor> mEditor;
};

#endif 

#endif
