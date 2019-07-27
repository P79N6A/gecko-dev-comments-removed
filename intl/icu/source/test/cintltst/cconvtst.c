













#include "cintltst.h"

void addTestConvert(TestNode**);
#include "nucnvtst.h"
void addTestConvertErrorCallBack(TestNode** root);
void addTestEuroRegression(TestNode** root);
void addTestConverterFallBack(TestNode** root);
void addExtraTests(TestNode** root);


U_CFUNC void
addBOCU1Tests(TestNode** root);

void addConvert(TestNode** root);

void addConvert(TestNode** root)
{
    addTestConvert(root);
    addTestNewConvert(root);
    addBOCU1Tests(root);
    addTestConvertErrorCallBack(root);
    addTestEuroRegression(root);
#if !UCONFIG_NO_LEGACY_CONVERSION
    addTestConverterFallBack(root);
#endif
    addExtraTests(root);
}
