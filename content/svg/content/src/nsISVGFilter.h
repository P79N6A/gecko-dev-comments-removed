



































#ifndef __NS_ISVGFILTER_H__
#define __NS_ISVGFILTER_H__

#define NS_ISVGFILTER_IID \
{ 0xbc1bf81a, 0x9765, 0x43c0, { 0xb3, 0x17, 0xd6, 0x91, 0x66, 0xcd, 0x3a, 0x30 } }


#define NS_FE_SOURCEGRAPHIC   0x01
#define NS_FE_SOURCEALPHA     0x02
#define NS_FE_BACKGROUNDIMAGE 0x04
#define NS_FE_BACKGROUNDALPHA 0x08
#define NS_FE_FILLPAINT       0x10
#define NS_FE_STROKEPAINT     0x20

class nsSVGFilterInstance;

class nsISVGFilter : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGFILTER_IID)

  
  
  NS_IMETHOD Filter(nsSVGFilterInstance *instance) = 0;

  
  NS_IMETHOD GetRequirements(PRUint32 *aRequirements) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGFilter, NS_ISVGFILTER_IID)

#endif
