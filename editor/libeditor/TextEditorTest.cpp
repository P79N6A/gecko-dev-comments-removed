




#ifdef DEBUG

#include "TextEditorTest.h"

#include <stdio.h>

#include "nsDebug.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIPlaintextEditor.h"
#include "nsISelection.h"
#include "nsLiteralString.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsStringFwd.h"

#define TEST_RESULT(r) { if (NS_FAILED(r)) {printf("FAILURE result=%X\n", static_cast<uint32_t>(r)); return r; } }
#define TEST_POINTER(p) { if (!p) {printf("FAILURE null pointer\n"); return NS_ERROR_NULL_POINTER; } }

TextEditorTest::TextEditorTest()
{
  printf("constructed a TextEditorTest\n");
}

TextEditorTest::~TextEditorTest()
{
  printf("destroyed a TextEditorTest\n");
}

void TextEditorTest::Run(nsIEditor *aEditor, int32_t *outNumTests, int32_t *outNumTestsFailed)
{
  if (!aEditor) return;
  mTextEditor = do_QueryInterface(aEditor);
  mEditor = do_QueryInterface(aEditor);
  RunUnitTest(outNumTests, outNumTestsFailed);
}

nsresult TextEditorTest::RunUnitTest(int32_t *outNumTests, int32_t *outNumTestsFailed)
{
  nsresult result;
  
  NS_ENSURE_TRUE(outNumTests && outNumTestsFailed, NS_ERROR_NULL_POINTER);
  
  *outNumTests = 0;
  *outNumTestsFailed = 0;
  
  result = InitDoc();
  TEST_RESULT(result);
  
  
  
  result = mTextEditor->InsertText(NS_LITERAL_STRING("1234567890abcdefghij1234567890"));
  TEST_RESULT(result);
  (*outNumTests)++;
  if (NS_FAILED(result))
    ++(*outNumTestsFailed);
  
  
  result = mTextEditor->InsertText(NS_LITERAL_STRING("Moreover, I am cognizant of the interrelatedness of all communities and states.  I cannot sit idly by in Atlanta and not be concerned about what happens in Birmingham.  Injustice anywhere is a threat to justice everywhere"));
  TEST_RESULT(result);
  (*outNumTests)++;
  if (NS_FAILED(result))
    ++(*outNumTestsFailed);

  result = TestInsertBreak();
  TEST_RESULT(result);
  (*outNumTests)++;
  if (NS_FAILED(result))
    ++(*outNumTestsFailed);

  result = TestTextProperties();
  TEST_RESULT(result);
  (*outNumTests)++;
  if (NS_FAILED(result))
    ++(*outNumTestsFailed);

  
  result = mEditor->Undo(12);
  TEST_RESULT(result);

  return result;
}

nsresult TextEditorTest::InitDoc()
{
  nsresult result = mEditor->SelectAll();
  TEST_RESULT(result);
  result = mEditor->DeleteSelection(nsIEditor::eNext, nsIEditor::eStrip);
  TEST_RESULT(result);
  return result;
}

nsresult TextEditorTest::TestInsertBreak()
{
  nsCOMPtr<nsISelection>selection;
  nsresult result = mEditor->GetSelection(getter_AddRefs(selection));
  TEST_RESULT(result);
  TEST_POINTER(selection.get());
  nsCOMPtr<nsIDOMNode>anchor;
  result = selection->GetAnchorNode(getter_AddRefs(anchor));
  TEST_RESULT(result);
  TEST_POINTER(anchor.get());
  selection->Collapse(anchor, 0);
  
  printf("inserting a break\n");
  result = mTextEditor->InsertLineBreak();
  TEST_RESULT(result);
  mEditor->DebugDumpContent();

  
  printf("inserting a second break\n");
  result = mTextEditor->InsertLineBreak();
  TEST_RESULT(result);
  mEditor->DebugDumpContent();
    
  return result;
}

nsresult TextEditorTest::TestTextProperties()
{
  nsCOMPtr<nsISelection>selection;
  nsresult result = mEditor->GetSelection(getter_AddRefs(selection));
  TEST_RESULT(result);
  TEST_POINTER(selection.get());
  nsCOMPtr<nsIDOMNode>anchor;
  result = selection->GetAnchorNode(getter_AddRefs(anchor));
  TEST_RESULT(result);
  TEST_POINTER(anchor.get());
  nsCOMPtr<nsIDOMNodeList>nodeList;
  result = anchor->GetChildNodes(getter_AddRefs(nodeList));
  TEST_RESULT(result);
  TEST_POINTER(nodeList.get());
  uint32_t count;
  nodeList->GetLength(&count);
  NS_ASSERTION(0!=count, "there are no nodes in the document!");
  nsCOMPtr<nsIDOMNode>textNode;
  result = nodeList->Item(count-2, getter_AddRefs(textNode));
  TEST_RESULT(result);
  TEST_POINTER(textNode.get());

  nsCOMPtr<nsIDOMCharacterData>textData;
  textData = do_QueryInterface(textNode);
  TEST_POINTER(textData.get());
  uint32_t length;
  textData->GetLength(&length);
  selection->Collapse(textNode, 0);
  selection->Extend(textNode, length);

  nsCOMPtr<nsIHTMLEditor> htmlEditor (do_QueryInterface(mTextEditor));
  NS_ENSURE_TRUE(htmlEditor, NS_ERROR_FAILURE);

  bool any = false;
  bool all = false;
  bool first=false;
  bool mixed=false;
  nsString fontFace;

  
  printf("set the whole first text node to serif\n");
  result = htmlEditor->SetInlineProperty(nsGkAtoms::font,
                                         NS_LITERAL_STRING("face"),
                                         NS_LITERAL_STRING("serif"));
  TEST_RESULT(result);
  result = htmlEditor->GetFontFaceState(&mixed, fontFace);
  TEST_RESULT(result);
  NS_ASSERTION(mixed==false,"mixed should be false");
  NS_ASSERTION(fontFace.IsEmpty(),"font face should be empty");

  printf("set the whole first text node to sans-serif\n");
  result = htmlEditor->SetInlineProperty(nsGkAtoms::font,
                                         NS_LITERAL_STRING("face"),
                                         NS_LITERAL_STRING("sans-serif"));
  TEST_RESULT(result);
  result = htmlEditor->GetFontFaceState(&mixed, fontFace);
  TEST_RESULT(result);
  NS_ASSERTION(mixed==false,"mixed should be false");
  NS_ASSERTION(fontFace.IsEmpty(),"font face should be empty");

  
  printf("set the whole first text node to cursive\n");
  result = htmlEditor->SetInlineProperty(nsGkAtoms::font,
                                         NS_LITERAL_STRING("face"),
                                         NS_LITERAL_STRING("cursive"));
  TEST_RESULT(result);
  result = htmlEditor->GetInlineProperty(nsGkAtoms::font,
                                         NS_LITERAL_STRING("face"),
                                         NS_LITERAL_STRING("fantasy"),
                                         &first, &any, &all);
  TEST_RESULT(result);
  NS_ASSERTION(false==first, "first should be false");
  NS_ASSERTION(false==any, "any should be false");
  NS_ASSERTION(false==all, "all should be false");
  selection->Collapse(textNode, 0);
  result = htmlEditor->GetInlineProperty(nsGkAtoms::font,
                                         NS_LITERAL_STRING("face"),
                                         NS_LITERAL_STRING("fantasy"),
                                         &first, &any, &all);
  TEST_RESULT(result);
  NS_ASSERTION(false==first, "first should be false");
  NS_ASSERTION(false==any, "any should be false");
  NS_ASSERTION(false==all, "all should be false");

  const nsAFlatString& empty = EmptyString();

  
  printf("set the whole first text node to bold\n");
  selection->Collapse(textNode, 0);
  selection->Extend(textNode, length);
  result = htmlEditor->GetInlineProperty(nsGkAtoms::b, empty, empty, &first,
                                         &any, &all);
  TEST_RESULT(result);
  NS_ASSERTION(false==first, "first should be false");
  NS_ASSERTION(false==any, "any should be false");
  NS_ASSERTION(false==all, "all should be false");
  result = htmlEditor->SetInlineProperty(nsGkAtoms::b, empty, empty);
  TEST_RESULT(result);
  result = htmlEditor->GetInlineProperty(nsGkAtoms::b, empty, empty, &first,
                                         &any, &all);
  TEST_RESULT(result);
  NS_ASSERTION(true==first, "first should be true");
  NS_ASSERTION(true==any, "any should be true");
  NS_ASSERTION(true==all, "all should be true");
  mEditor->DebugDumpContent();

  
  printf("set the whole first text node to not bold\n");
  result = htmlEditor->RemoveInlineProperty(nsGkAtoms::b, empty);
  TEST_RESULT(result);
  result = htmlEditor->GetInlineProperty(nsGkAtoms::b, empty, empty, &first,
                                         &any, &all);
  TEST_RESULT(result);
  NS_ASSERTION(false==first, "first should be false");
  NS_ASSERTION(false==any, "any should be false");
  NS_ASSERTION(false==all, "all should be false");
  mEditor->DebugDumpContent();

  
  printf("set the first text node (1, length-1) to bold and italic, and (2, length-1) to underline.\n");
  selection->Collapse(textNode, 1);
  selection->Extend(textNode, length-1);
  result = htmlEditor->SetInlineProperty(nsGkAtoms::b, empty, empty);
  TEST_RESULT(result);
  result = htmlEditor->GetInlineProperty(nsGkAtoms::b, empty, empty, &first,
                                         &any, &all);
  TEST_RESULT(result);
  NS_ASSERTION(true==first, "first should be true");
  NS_ASSERTION(true==any, "any should be true");
  NS_ASSERTION(true==all, "all should be true");
  mEditor->DebugDumpContent();
  
  result = htmlEditor->SetInlineProperty(nsGkAtoms::i, empty, empty);
  TEST_RESULT(result);
  result = htmlEditor->GetInlineProperty(nsGkAtoms::i, empty, empty, &first,
                                         &any, &all);
  TEST_RESULT(result);
  NS_ASSERTION(true==first, "first should be true");
  NS_ASSERTION(true==any, "any should be true");
  NS_ASSERTION(true==all, "all should be true");
  result = htmlEditor->GetInlineProperty(nsGkAtoms::b, empty, empty, &first,
                                         &any, &all);
  TEST_RESULT(result);
  NS_ASSERTION(true==first, "first should be true");
  NS_ASSERTION(true==any, "any should be true");
  NS_ASSERTION(true==all, "all should be true");
  mEditor->DebugDumpContent();

  result = htmlEditor->SetInlineProperty(nsGkAtoms::u, empty, empty);
  TEST_RESULT(result);
  result = htmlEditor->GetInlineProperty(nsGkAtoms::u, empty, empty, &first,
                                         &any, &all);
  TEST_RESULT(result);
  NS_ASSERTION(true==first, "first should be true");
  NS_ASSERTION(true==any, "any should be true");
  NS_ASSERTION(true==all, "all should be true");
  mEditor->DebugDumpContent();

  return result;
}



#endif


