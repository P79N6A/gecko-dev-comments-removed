




































#ifndef nsMathMLOperators_h___
#define nsMathMLOperators_h___

#include "nsCoord.h"

typedef PRUint32 nsOperatorFlags;
typedef PRInt32 nsStretchDirection;

#define NS_STRETCH_DIRECTION_UNSUPPORTED  -1
#define NS_STRETCH_DIRECTION_DEFAULT       0
#define NS_STRETCH_DIRECTION_HORIZONTAL    1
#define NS_STRETCH_DIRECTION_VERTICAL      2



#define NS_MATHML_OPERATOR_MUTABLE            (1<<31)
#define NS_MATHML_OPERATOR_EMBELLISH_ANCESTOR (1<<30)
#define NS_MATHML_OPERATOR_EMBELLISH_ISOLATED (1<<29)
#define NS_MATHML_OPERATOR_CENTERED           (1<<28)
#define NS_MATHML_OPERATOR_INVISIBLE          (1<<27)



#define NS_MATHML_OPERATOR_FORM  0x3 // the very last two bits tell us the form
#define     NS_MATHML_OPERATOR_FORM_INFIX      1
#define     NS_MATHML_OPERATOR_FORM_PREFIX     2
#define     NS_MATHML_OPERATOR_FORM_POSTFIX    3
#define NS_MATHML_OPERATOR_STRETCHY 0xC // the 2 penultimate last bits are used
#define     NS_MATHML_OPERATOR_STRETCHY_VERT  (1<<2)
#define     NS_MATHML_OPERATOR_STRETCHY_HORIZ (1<<3)
#define NS_MATHML_OPERATOR_FENCE              (1<<4)
#define NS_MATHML_OPERATOR_ACCENT             (1<<5)
#define NS_MATHML_OPERATOR_LARGEOP            (1<<6)
#define NS_MATHML_OPERATOR_SEPARATOR          (1<<7)
#define NS_MATHML_OPERATOR_MOVABLELIMITS      (1<<8)
#define NS_MATHML_OPERATOR_SYMMETRIC          (1<<9)



#define NS_MATHML_OPERATOR_MINSIZE_EXPLICIT   (1<<10)
#define NS_MATHML_OPERATOR_MAXSIZE_EXPLICIT   (1<<11)
#define NS_MATHML_OPERATOR_LEFTSPACE_ATTR     (1<<12)
#define NS_MATHML_OPERATOR_RIGHTSPACE_ATTR    (1<<13)


class nsMathMLOperators {
public:
  static void AddRefTable(void);
  static void ReleaseTable(void);
  static void CleanUp();

  
  
  
  
  
  
  
  
  static PRBool
  LookupOperator(const nsString&       aOperator,
                 const nsOperatorFlags aForm,
                 nsOperatorFlags*      aFlags,
                 float*                aLeftSpace,
                 float*                aRightSpace);

   
   
   
   
   
   
   static void
   LookupOperators(const nsString&       aOperator,
                   nsOperatorFlags*      aFlags,
                   float*                aLeftSpace,
                   float*                aRightSpace);

  
  
  static PRBool
  IsMutableOperator(const nsString& aOperator);

  
  
  static PRInt32 CountStretchyOperator();
  static PRInt32 FindStretchyOperator(PRUnichar aOperator);
  static nsStretchDirection GetStretchyDirectionAt(PRInt32 aIndex);
  static void DisableStretchyOperatorAt(PRInt32 aIndex);


  
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
  static PRBool LookupInvariantChar(PRUnichar     aChar,
                                    eMATHVARIANT* aType = nsnull);
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

#define NS_MATHML_OPERATOR_GET_STRETCHY_DIR(_flags) \
  ((_flags) & NS_MATHML_OPERATOR_STRETCHY)

#define NS_MATHML_OPERATOR_FORM_IS_INFIX(_flags) \
  (NS_MATHML_OPERATOR_FORM_INFIX == ((_flags) & NS_MATHML_OPERATOR_FORM_INFIX))

#define NS_MATHML_OPERATOR_FORM_IS_PREFIX(_flags) \
  (NS_MATHML_OPERATOR_FORM_PREFIX == ((_flags) & NS_MATHML_OPERATOR_FORM_PREFIX))

#define NS_MATHML_OPERATOR_FORM_IS_POSTFIX(_flags) \
  (NS_MATHML_OPERATOR_FORM_POSTFIX == ((_flags) & NS_MATHML_OPERATOR_FORM_POSTFIX ))

#define NS_MATHML_OPERATOR_IS_STRETCHY(_flags) \
  (0 != ((_flags) & NS_MATHML_OPERATOR_STRETCHY))

#define NS_MATHML_OPERATOR_IS_STRETCHY_VERT(_flags) \
  (NS_MATHML_OPERATOR_STRETCHY_VERT == ((_flags) & NS_MATHML_OPERATOR_STRETCHY_VERT))

#define NS_MATHML_OPERATOR_IS_STRETCHY_HORIZ(_flags) \
  (NS_MATHML_OPERATOR_STRETCHY_HORIZ == ((_flags) & NS_MATHML_OPERATOR_STRETCHY_HORIZ))

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

#define NS_MATHML_OPERATOR_MINSIZE_IS_EXPLICIT(_flags) \
  (NS_MATHML_OPERATOR_MINSIZE_EXPLICIT == ((_flags) & NS_MATHML_OPERATOR_MINSIZE_EXPLICIT))

#define NS_MATHML_OPERATOR_MAXSIZE_IS_EXPLICIT(_flags) \
  (NS_MATHML_OPERATOR_MAXSIZE_EXPLICIT == ((_flags) & NS_MATHML_OPERATOR_MAXSIZE_EXPLICIT))

#define NS_MATHML_OPERATOR_HAS_LEFTSPACE_ATTR(_flags) \
  (NS_MATHML_OPERATOR_LEFTSPACE_ATTR == ((_flags) & NS_MATHML_OPERATOR_LEFTSPACE_ATTR))

#define NS_MATHML_OPERATOR_HAS_RIGHTSPACE_ATTR(_flags) \
  (NS_MATHML_OPERATOR_RIGHTSPACE_ATTR == ((_flags) & NS_MATHML_OPERATOR_RIGHTSPACE_ATTR))

#endif 
