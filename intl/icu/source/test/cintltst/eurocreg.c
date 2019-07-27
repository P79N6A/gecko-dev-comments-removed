




#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/ctest.h"
#include "unicode/ucnv.h"

void TestEuroRegression(void);
void addTestEuroRegression(TestNode** root);

#if !UCONFIG_NO_LEGACY_CONVERSION
void addTestEuroRegression(TestNode** root)
{
    addTest(root, &TestEuroRegression, "tsconv/eurocreg/TestEuroRegression");
}






























static const char convertersToCheck[][15] = { 
  "cp1250",
  "cp1251",
  "cp1252",
  "cp1254",
  "cp1255",
  "cp1256",
  "cp1257",
  "cp1258",
  "ibm1140",
  "ibm1142",
  "ibm1143",
  "ibm1144",
  "ibm1145",
  "ibm1146",
  "ibm1147",
  "ibm1148",
  "ibm1149",
  "ibm1153",
  "ibm1154",
  "ibm1155",
  "ibm1156",
  "ibm1157",
  "ibm1158",
   
  "ibm12712",
  "ibm16804",
  "ibm-1160",
  "ibm-1162",
  "ibm-1164",

  "ibm-858", 
  
   
  "ibm-12712", 
  "ibm-4899", 
  "ibm-867", 
  "cp1258",
  "windows-950",
  "cp1253",
  

  "ibm-4971",
   
  
   
   
   


  "ibm-902", 
  "ibm-901", 
   
  

  

  "ibm-5123", 
  
  
  "ibm-1364",
  
  "cp1363",
  

  "gb18030",
  ""};

UBool isEuroAware(UConverter*);

void TestEuroRegression()
{
    int32_t i=0;

    do 
    {
        UErrorCode err = U_ZERO_ERROR;
        UConverter* myConv =  ucnv_open(convertersToCheck[i], &err);
        if (U_FAILURE(err)&&convertersToCheck[i][0])
            log_data_err("%s  \tMISSING [%s]\n", convertersToCheck[i], u_errorName(err));
        else 
        {
            if (isEuroAware(myConv))
                log_verbose("%s  \tsupports euro\n", convertersToCheck[i]);
            else
                log_err("%s  \tDOES NOT support euro\n", convertersToCheck[i]);
            ucnv_close(myConv);
        }
    } while (convertersToCheck[++i][0]);
}

UBool isEuroAware(UConverter* myConv)
{
    static const UChar euroString[2] = { 0x20AC, 0x0000 };
    char target[20];
    UChar euroBack[2];
    int32_t targetSize, euroBackSize;
    UErrorCode err = U_ZERO_ERROR;
    

    targetSize = ucnv_fromUChars(myConv,
            target,
            sizeof(target),
            euroString,
            -1,
            &err);
    if (U_FAILURE(err))
    {
      log_err("Failure Occured in ucnv_fromUChars euro roundtrip test\n");
      return FALSE;
    }
    euroBackSize = ucnv_toUChars(myConv,
            euroBack,
            2,
            target,
            targetSize,
            &err);
    (void)euroBackSize;    
    if (U_FAILURE(err))
    {
        log_err("Failure Occured in ucnv_toUChars euro roundtrip test\n");
        return FALSE;
    }
    if (u_strcmp(euroString, euroBack)) 
    {
        
        return FALSE;
    }
    else 
    {
        
        return TRUE;
    }

}
#else
void addTestEuroRegression(TestNode** root)
{
    
}
#endif
