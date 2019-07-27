




#ifndef __TextEditorTest_h__
#define __TextEditorTest_h__

#include "nscore.h"

class nsIEditor;
class nsIPlaintextEditor;
#ifdef DEBUG

#include "nsCOMPtr.h"

class TextEditorTest
{
public:

  void Run(nsIEditor *aEditor, int32_t *outNumTests, int32_t *outNumTestsFailed);
  TextEditorTest();
  ~TextEditorTest();

protected:

  
  nsresult InitDoc();

  nsresult RunUnitTest(int32_t *outNumTests, int32_t *outNumTestsFailed);

  nsresult TestInsertBreak();

  nsresult TestTextProperties();

  nsCOMPtr<nsIPlaintextEditor> mTextEditor;
  nsCOMPtr<nsIEditor> mEditor;
};

#endif 

#endif
