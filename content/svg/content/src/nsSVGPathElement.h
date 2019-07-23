





































#ifndef __NS_SVGPATHELEMENT_H__
#define __NS_SVGPATHELEMENT_H__

#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGPathElement.h"
#include "nsIDOMSVGAnimatedPathData.h"
#include "nsSVGNumber2.h"
#include "gfxPath.h"

class gfxContext;

class nsSVGPathList
{
friend class nsSVGPathDataParserToInternal;

public:
  enum { MOVETO, LINETO, CURVETO, CLOSEPATH };
  nsSVGPathList() : mArguments(nsnull), mNumCommands(0), mNumArguments(0) {}
  ~nsSVGPathList() { Clear(); }
  void Playback(gfxContext *aCtx);

protected:
  void Clear();

  float   *mArguments;
  PRUint32 mNumCommands;
  PRUint32 mNumArguments;
};

typedef nsSVGPathGeometryElement nsSVGPathElementBase;

class nsSVGPathElement : public nsSVGPathElementBase,
                         public nsIDOMSVGPathElement,
                         public nsIDOMSVGAnimatedPathData
{
friend class nsSVGPathFrame;

protected:
  friend nsresult NS_NewSVGPathElement(nsIContent **aResult,
                                       nsINodeInfo *aNodeInfo);
  nsSVGPathElement(nsINodeInfo *aNodeInfo);
  virtual ~nsSVGPathElement();

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGPATHELEMENT
  NS_DECL_NSIDOMSVGANIMATEDPATHDATA

  
  NS_FORWARD_NSIDOMNODE(nsSVGPathElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGPathElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGPathElementBase::)

  
  NS_IMETHODIMP_(PRBool) IsAttributeMapped(const nsIAtom* name) const;

  
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);

  
  virtual PRBool IsDependentAttribute(nsIAtom *aName);
  virtual PRBool IsMarkable();
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx);

  virtual already_AddRefed<gfxFlattenedPath> GetFlattenedPath(nsIDOMSVGMatrix *aMatrix);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  virtual NumberAttributesInfo GetNumberInfo();
  virtual nsresult BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify);

  
  nsresult CreatePathSegList();

  nsCOMPtr<nsIDOMSVGPathSegList> mSegments;
  nsSVGNumber2 mPathLength;
  static NumberInfo sNumberInfo;
  nsSVGPathList mPathData;
};

#endif
