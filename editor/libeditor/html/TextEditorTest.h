




































#ifndef __TextEditorTest_h__
#define __TextEditorTest_h__

#ifdef NS_DEBUG

#include "nsCOMPtr.h"
#include "nsIEditor.h"
#include "nsIPlaintextEditor.h"

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
