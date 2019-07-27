




#ifndef _nshtmlobjectresizer__h
#define _nshtmlobjectresizer__h

#include "nsIDOMEventListener.h"
#include "nsISelectionListener.h"
#include "nsISupportsImpl.h"
#include "nsIWeakReferenceUtils.h"
#include "nsLiteralString.h"

class nsIHTMLEditor;

#define kTopLeft       NS_LITERAL_STRING("nw")
#define kTop           NS_LITERAL_STRING("n")
#define kTopRight      NS_LITERAL_STRING("ne")
#define kLeft          NS_LITERAL_STRING("w")
#define kRight         NS_LITERAL_STRING("e")
#define kBottomLeft    NS_LITERAL_STRING("sw")
#define kBottom        NS_LITERAL_STRING("s")
#define kBottomRight   NS_LITERAL_STRING("se")





class ResizerSelectionListener : public nsISelectionListener
{
public:

  explicit ResizerSelectionListener(nsIHTMLEditor * aEditor);
  void Reset();

  
  NS_DECL_ISUPPORTS

  NS_DECL_NSISELECTIONLISTENER

protected:
  virtual ~ResizerSelectionListener();

  nsWeakPtr mEditor;
};





class ResizerMouseMotionListener : public nsIDOMEventListener
{
public:
  explicit ResizerMouseMotionListener(nsIHTMLEditor * aEditor);


  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMEVENTLISTENER

 protected:
  virtual ~ResizerMouseMotionListener();

  nsWeakPtr mEditor;

};





class DocumentResizeEventListener: public nsIDOMEventListener
{
public:
  explicit DocumentResizeEventListener(nsIHTMLEditor * aEditor);

  
  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMEVENTLISTENER

 protected:
  virtual ~DocumentResizeEventListener();
  nsWeakPtr mEditor;

};

#endif 
