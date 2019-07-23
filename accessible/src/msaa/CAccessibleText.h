







































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


#define FORWARD_IACCESSIBLETEXT(Class)                                         \
virtual HRESULT STDMETHODCALLTYPE addSelection(long startOffset,               \
                                               long endOffset)                 \
{                                                                              \
  return Class::addSelection(startOffset, endOffset);                          \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_attributes(long offset,                  \
                                                 long *startOffset,            \
                                                 long *endOffset,              \
                                                 BSTR *textAttributes)         \
{                                                                              \
  return Class::get_attributes(offset, startOffset, endOffset, textAttributes);\
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_caretOffset(long *offset)                \
{                                                                              \
  return Class::get_caretOffset(offset);                                       \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_characterExtents(long offset,            \
                                                       enum IA2CoordinateType coordType,\
                                                       long *x,                \
                                                       long *y,                \
                                                       long *width,            \
                                                       long *height)           \
{                                                                              \
  return Class::get_characterExtents(offset, coordType, x, y, width, height);  \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_nSelections(long *nSelections)           \
{                                                                              \
  return Class::get_nSelections(nSelections);                                  \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_offsetAtPoint(long x,                    \
                                                    long y,                    \
                                                    enum IA2CoordinateType coordType,\
                                                    long *offset)              \
{                                                                              \
  return Class::get_offsetAtPoint(x, y, coordType, offset);                    \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_selection(long selectionIndex,           \
                                                long *startOffset,             \
                                                long *endOffset)               \
{                                                                              \
  return Class::get_selection(selectionIndex, startOffset, endOffset);         \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_text(long startOffset,                   \
                                           long endOffset,                     \
                                           BSTR *text)                         \
{                                                                              \
  return Class::get_text(startOffset, endOffset, text);                        \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_textBeforeOffset(long offset,            \
                                                       enum IA2TextBoundaryType boundaryType,\
                                                       long *startOffset,      \
                                                       long *endOffset,        \
                                                       BSTR *text)             \
{                                                                              \
  return Class::get_textBeforeOffset(offset, boundaryType,                     \
                                     startOffset, endOffset, text);            \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_textAfterOffset(long offset,             \
                                                      enum IA2TextBoundaryType boundaryType,\
                                                      long *startOffset,       \
                                                      long *endOffset,         \
                                                      BSTR *text)              \
{                                                                              \
  return Class::get_textAfterOffset(offset, boundaryType,                      \
                                    startOffset, endOffset, text);             \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_textAtOffset(long offset,                \
                                                   enum IA2TextBoundaryType boundaryType,\
                                                   long *startOffset,          \
                                                   long *endOffset,            \
                                                   BSTR *text)                 \
{                                                                              \
  return Class::get_textAtOffset(offset, boundaryType,                         \
                                 startOffset, endOffset, text);                \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE removeSelection(long selectionIndex)         \
{                                                                              \
  return Class::removeSelection(selectionIndex);                               \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE setCaretOffset(long offset)                  \
{                                                                              \
  return Class::setCaretOffset(offset);                                        \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE setSelection(long selectionIndex,            \
                                               long startOffset,               \
                                               long endOffset)                 \
{                                                                              \
  return Class::setSelection(selectionIndex, startOffset, endOffset);          \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_nCharacters(long *nCharacters)           \
{                                                                              \
  return Class::get_nCharacters(nCharacters);                                  \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE scrollSubstringTo(long startIndex,           \
                                                    long endIndex,             \
                                                    enum IA2ScrollType scrollType)\
{                                                                              \
  return Class::scrollSubstringTo(startIndex, endIndex, scrollType);           \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE scrollSubstringToPoint(long startIndex,      \
                                                         long endIndex,        \
                                                         enum IA2CoordinateType coordinateType,\
                                                         long x,               \
                                                         long y)               \
{                                                                              \
  return Class::scrollSubstringToPoint(startIndex, endIndex,                   \
                                       coordinateType, x, y);                  \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_newText(IA2TextSegment *newText)         \
{                                                                              \
  return Class::get_newText(newText);                                          \
}                                                                              \
                                                                               \
virtual HRESULT STDMETHODCALLTYPE get_oldText(IA2TextSegment *oldText)         \
{                                                                              \
  return Class::get_oldText(oldText);                                          \
}                                                                              \

#endif

