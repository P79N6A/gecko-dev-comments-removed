







































#ifndef _ACCESSIBLE_TEXT_H
#define _ACCESSIBLE_TEXT_H

#include "nsISupports.h"
#include "nsIAccessibleText.h"

#include "AccessibleText.h"

class CAccessibleText: public nsISupports,
                       public IAccessibleText
{
public:

  
  STDMETHODIMP QueryInterface(REFIID, void**);

  
  virtual HRESULT STDMETHODCALLTYPE addSelection(
       long startOffset,
       long endOffset);

  virtual  HRESULT STDMETHODCALLTYPE get_attributes(
       long offset,
       long *startOffset,
       long *endOffset,
       BSTR *textAttributes);

  virtual  HRESULT STDMETHODCALLTYPE get_caretOffset(
       long *offset);

  virtual  HRESULT STDMETHODCALLTYPE get_characterExtents(
       long offset,
       enum IA2CoordinateType coordType,
       long *x,
       long *y,
       long *width,
       long *height);

  virtual  HRESULT STDMETHODCALLTYPE get_nSelections(
       long *nSelections);

  virtual  HRESULT STDMETHODCALLTYPE get_offsetAtPoint(
       long x,
       long y,
       enum IA2CoordinateType coordType,
       long *offset);

  virtual  HRESULT STDMETHODCALLTYPE get_selection(
       long selectionIndex,
       long *startOffset,
       long *endOffset);

  virtual  HRESULT STDMETHODCALLTYPE get_text(
       long startOffset,
       long endOffset,
       BSTR *text);

  virtual  HRESULT STDMETHODCALLTYPE get_textBeforeOffset(
       long offset,
       enum IA2TextBoundaryType boundaryType,
       long *startOffset,
       long *endOffset,
       BSTR *text);

  virtual  HRESULT STDMETHODCALLTYPE get_textAfterOffset(
       long offset,
       enum IA2TextBoundaryType boundaryType,
       long *startOffset,
       long *endOffset,
       BSTR *text);

  virtual  HRESULT STDMETHODCALLTYPE get_textAtOffset(
       long offset,
       enum IA2TextBoundaryType boundaryType,
       long *startOffset,
       long *endOffset,
       BSTR *text);

  virtual HRESULT STDMETHODCALLTYPE removeSelection(
       long selectionIndex);

  virtual HRESULT STDMETHODCALLTYPE setCaretOffset(
       long offset);

  virtual HRESULT STDMETHODCALLTYPE setSelection(
       long selectionIndex,
       long startOffset,
       long endOffset);

  virtual  HRESULT STDMETHODCALLTYPE get_nCharacters(
       long *nCharacters);

  virtual HRESULT STDMETHODCALLTYPE scrollSubstringTo(
       long startIndex,
       long endIndex,
       enum IA2ScrollType scrollType);

  virtual HRESULT STDMETHODCALLTYPE scrollSubstringToPoint(
       long startIndex,
       long endIndex,
       enum IA2CoordinateType coordinateType,
       long x,
       long y);

  virtual  HRESULT STDMETHODCALLTYPE get_newText(
       IA2TextSegment *newText);

  virtual  HRESULT STDMETHODCALLTYPE get_oldText(
       IA2TextSegment *oldText);

protected:
  virtual nsresult GetModifiedText(PRBool aGetInsertedText, nsAString& aText,
                                   PRUint32 *aStartOffset,
                                   PRUint32 *aEndOffset) = 0;

private:
  HRESULT GetModifiedText(PRBool aGetInsertedText, IA2TextSegment *aNewText);
  nsAccessibleTextBoundary GetGeckoTextBoundary(enum IA2TextBoundaryType coordinateType);
};

#endif

