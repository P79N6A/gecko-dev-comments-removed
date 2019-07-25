







































#ifndef nsMathMLOperators_h___
#define nsMathMLOperators_h___

#include "nsCoord.h"

enum nsStretchDirection {
  NS_STRETCH_DIRECTION_UNSUPPORTED = -1,
  NS_STRETCH_DIRECTION_DEFAULT     =  0,
  NS_STRETCH_DIRECTION_HORIZONTAL  =  1,
  NS_STRETCH_DIRECTION_VERTICAL    =  2
};

typedef PRUint32 nsOperatorFlags;
enum {
  
  NS_MATHML_OPERATOR_MUTABLE            = 1<<30,
  NS_MATHML_OPERATOR_EMBELLISH_ANCESTOR = 1<<29,
  NS_MATHML_OPERATOR_EMBELLISH_ISOLATED = 1<<28,
  NS_MATHML_OPERATOR_CENTERED           = 1<<27,
  NS_MATHML_OPERATOR_INVISIBLE          = 1<<26,

  

  
  NS_MATHML_OPERATOR_FORM               = 0x3,
  NS_MATHML_OPERATOR_FORM_INFIX         = 1,
  NS_MATHML_OPERATOR_FORM_PREFIX        = 2,
  NS_MATHML_OPERATOR_FORM_POSTFIX       = 3,

  
  NS_MATHML_OPERATOR_DIRECTION            = 0x3<<2,
  NS_MATHML_OPERATOR_DIRECTION_HORIZONTAL = 1<<2,
  NS_MATHML_OPERATOR_DIRECTION_VERTICAL   = 2<<2,

  
  NS_MATHML_OPERATOR_STRETCHY           = 1<<4,
  NS_MATHML_OPERATOR_FENCE              = 1<<5,
  NS_MATHML_OPERATOR_ACCENT             = 1<<6,
  NS_MATHML_OPERATOR_LARGEOP            = 1<<7,
  NS_MATHML_OPERATOR_SEPARATOR          = 1<<8,
  NS_MATHML_OPERATOR_MOVABLELIMITS      = 1<<9,
  NS_MATHML_OPERATOR_SYMMETRIC          = 1<<10,
  NS_MATHML_OPERATOR_INTEGRAL           = 1<<11,
  NS_MATHML_OPERATOR_MIRRORABLE         = 1<<12,

  
  NS_MATHML_OPERATOR_MINSIZE_ABSOLUTE   = 1<<13,
  NS_MATHML_OPERATOR_MAXSIZE_ABSOLUTE   = 1<<14,
  NS_MATHML_OPERATOR_LSPACE_ATTR     = 1<<15,
  NS_MATHML_OPERATOR_RSPACE_ATTR    = 1<<16
};

#define NS_MATHML_OPERATOR_SIZE_INFINITY NS_IEEEPositiveInfinity()


enum eMATHVARIANT {
  eMATHVARIANT_NONE = -1,
  eMATHVARIANT_normal = 0,
  eMATHVARIANT_bold,
  eMATHVARIANT_italic,
  eMATHVARIANT_bold_italic,
  eMATHVARIANT_sans_serif,
  eMATHVARIANT_bold_sans_serif,
  eMATHVARIANT_sans_serif_italic,
  eMATHVARIANT_sans_serif_bold_italic,
  eMATHVARIANT_monospace,
  eMATHVARIANT_script,
  eMATHVARIANT_bold_script,
  eMATHVARIANT_fraktur,
  eMATHVARIANT_bold_fraktur,
  eMATHVARIANT_double_struck,
  eMATHVARIANT_COUNT
};

class nsMathMLOperators {
public:
  static void AddRefTable(void);
  static void ReleaseTable(void);
  static void CleanUp();

  
  
  
  
  
  
  
  
  static bool
  LookupOperator(const nsString&       aOperator,
                 const nsOperatorFlags aForm,
                 nsOperatorFlags*      aFlags,
                 float*                aLeadingSpace,
                 float*                aTrailingSpace);

   
   
   
   
   
   
   
   static void
   LookupOperators(const nsString&       aOperator,
                   nsOperatorFlags*      aFlags,
                   float*                aLeadingSpace,
                   float*                aTrailingSpace);

  
  
  static bool
  IsMutableOperator(const nsString& aOperator);

  
  static bool
  IsMirrorableOperator(const nsString& aOperator);

  
  static nsStretchDirection GetStretchyDirection(const nsString& aOperator);

  
  
  
  static eMATHVARIANT LookupInvariantChar(const nsAString& aChar);

  
  
  
  
  
  static const nsDependentSubstring
  TransformVariantChar(const PRUnichar& aChar, eMATHVARIANT aVariant);
};




#define NS_MATHML_OPERATOR_IS_MUTABLE(_flags) \
  (NS_MATHML_OPERATOR_MUTABLE == ((_flags) & NS_MATHML_OPERATOR_MUTABLE))

#define NS_MATHML_OPERATOR_HAS_EMBELLISH_ANCESTOR(_flags) \
  (NS_MATHML_OPERATOR_EMBELLISH_ANCESTOR == ((_flags) & NS_MATHML_OPERATOR_EMBELLISH_ANCESTOR))

#define NS_MATHML_OPERATOR_EMBELLISH_IS_ISOLATED(_flags) \
  (NS_MATHML_OPERATOR_EMBELLISH_ISOLATED == ((_flags) & NS_MATHML_OPERATOR_EMBELLISH_ISOLATED))

#define NS_MATHML_OPERATOR_IS_CENTERED(_flags) \
  (NS_MATHML_OPERATOR_CENTERED == ((_flags) & NS_MATHML_OPERATOR_CENTERED))

#define NS_MATHML_OPERATOR_IS_INVISIBLE(_flags) \
  (NS_MATHML_OPERATOR_INVISIBLE == ((_flags) & NS_MATHML_OPERATOR_INVISIBLE))

#define NS_MATHML_OPERATOR_GET_FORM(_flags) \
  ((_flags) & NS_MATHML_OPERATOR_FORM)

#define NS_MATHML_OPERATOR_GET_DIRECTION(_flags) \
  ((_flags) & NS_MATHML_OPERATOR_DIRECTION)

#define NS_MATHML_OPERATOR_FORM_IS_INFIX(_flags) \
  (NS_MATHML_OPERATOR_FORM_INFIX == ((_flags) & NS_MATHML_OPERATOR_FORM))

#define NS_MATHML_OPERATOR_FORM_IS_PREFIX(_flags) \
  (NS_MATHML_OPERATOR_FORM_PREFIX == ((_flags) & NS_MATHML_OPERATOR_FORM))

#define NS_MATHML_OPERATOR_FORM_IS_POSTFIX(_flags) \
  (NS_MATHML_OPERATOR_FORM_POSTFIX == ((_flags) & NS_MATHML_OPERATOR_FORM))

#define NS_MATHML_OPERATOR_IS_DIRECTION_VERTICAL(_flags) \
  (NS_MATHML_OPERATOR_DIRECTION_VERTICAL == ((_flags) & NS_MATHML_OPERATOR_DIRECTION))

#define NS_MATHML_OPERATOR_IS_DIRECTION_HORIZONTAL(_flags) \
  (NS_MATHML_OPERATOR_DIRECTION_HORIZONTAL == ((_flags) & NS_MATHML_OPERATOR_DIRECTION))

#define NS_MATHML_OPERATOR_IS_STRETCHY(_flags) \
  (NS_MATHML_OPERATOR_STRETCHY == ((_flags) & NS_MATHML_OPERATOR_STRETCHY))

#define NS_MATHML_OPERATOR_IS_FENCE(_flags) \
  (NS_MATHML_OPERATOR_FENCE == ((_flags) & NS_MATHML_OPERATOR_FENCE))

#define NS_MATHML_OPERATOR_IS_ACCENT(_flags) \
  (NS_MATHML_OPERATOR_ACCENT == ((_flags) & NS_MATHML_OPERATOR_ACCENT))

#define NS_MATHML_OPERATOR_IS_LARGEOP(_flags) \
  (NS_MATHML_OPERATOR_LARGEOP == ((_flags) & NS_MATHML_OPERATOR_LARGEOP))

#define NS_MATHML_OPERATOR_IS_SEPARATOR(_flags) \
  (NS_MATHML_OPERATOR_SEPARATOR == ((_flags) & NS_MATHML_OPERATOR_SEPARATOR))

#define NS_MATHML_OPERATOR_IS_MOVABLELIMITS(_flags) \
  (NS_MATHML_OPERATOR_MOVABLELIMITS == ((_flags) & NS_MATHML_OPERATOR_MOVABLELIMITS))

#define NS_MATHML_OPERATOR_IS_SYMMETRIC(_flags) \
  (NS_MATHML_OPERATOR_SYMMETRIC == ((_flags) & NS_MATHML_OPERATOR_SYMMETRIC))

#define NS_MATHML_OPERATOR_IS_INTEGRAL(_flags) \
  (NS_MATHML_OPERATOR_INTEGRAL == ((_flags) & NS_MATHML_OPERATOR_INTEGRAL))

#define NS_MATHML_OPERATOR_IS_MIRRORABLE(_flags) \
  (NS_MATHML_OPERATOR_MIRRORABLE == ((_flags) & NS_MATHML_OPERATOR_MIRRORABLE))

#define NS_MATHML_OPERATOR_MINSIZE_IS_ABSOLUTE(_flags) \
  (NS_MATHML_OPERATOR_MINSIZE_ABSOLUTE == ((_flags) & NS_MATHML_OPERATOR_MINSIZE_ABSOLUTE))

#define NS_MATHML_OPERATOR_MAXSIZE_IS_ABSOLUTE(_flags) \
  (NS_MATHML_OPERATOR_MAXSIZE_ABSOLUTE == ((_flags) & NS_MATHML_OPERATOR_MAXSIZE_ABSOLUTE))

#define NS_MATHML_OPERATOR_HAS_LSPACE_ATTR(_flags) \
  (NS_MATHML_OPERATOR_LSPACE_ATTR == ((_flags) & NS_MATHML_OPERATOR_LSPACE_ATTR))

#define NS_MATHML_OPERATOR_HAS_RSPACE_ATTR(_flags) \
  (NS_MATHML_OPERATOR_RSPACE_ATTR == ((_flags) & NS_MATHML_OPERATOR_RSPACE_ATTR))

#endif 
