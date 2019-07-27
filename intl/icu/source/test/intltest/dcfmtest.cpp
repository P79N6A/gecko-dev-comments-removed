











#include "intltest.h"

#if !UCONFIG_NO_FORMATTING && !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/regex.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/locid.h"
#include "cmemory.h"
#include "dcfmtest.h"
#include "util.h"
#include "cstring.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if !defined(_MSC_VER)
namespace std { class type_info; } 
#endif

#include <string>
#include <iostream>






DecimalFormatTest::DecimalFormatTest()
{
}


DecimalFormatTest::~DecimalFormatTest()
{
}



void DecimalFormatTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    if (exec) logln("TestSuite DecimalFormatTest: ");
    switch (index) {

#if !UCONFIG_NO_FILE_IO
        case 0: name = "DataDrivenTests";
            if (exec) DataDrivenTests();
            break;
#else
        case 0: name = "skip";
            break;
#endif

        default: name = "";
            break; 
    }
}







#define DF_CHECK_STATUS {if (U_FAILURE(status)) \
    {dataerrln("DecimalFormatTest failure at line %d.  status=%s", \
    __LINE__, u_errorName(status)); return 0;}}

#define DF_ASSERT(expr) {if ((expr)==FALSE) {errln("DecimalFormatTest failure at line %d.\n", __LINE__);};}

#define DF_ASSERT_FAIL(expr, errcode) {UErrorCode status=U_ZERO_ERROR; (expr);\
if (status!=errcode) {dataerrln("DecimalFormatTest failure at line %d.  Expected status=%s, got %s", \
    __LINE__, u_errorName(errcode), u_errorName(status));};}

#define DF_CHECK_STATUS_L(line) {if (U_FAILURE(status)) {errln( \
    "DecimalFormatTest failure at line %d, from %d.  status=%d\n",__LINE__, (line), status); }}

#define DF_ASSERT_L(expr, line) {if ((expr)==FALSE) { \
    errln("DecimalFormatTest failure at line %d, from %d.", __LINE__, (line)); return;}}









class InvariantStringPiece: public StringPiece {
  public:
    InvariantStringPiece(const UnicodeString &s);
    ~InvariantStringPiece() {};
  private:
    MaybeStackArray<char, 20>  buf;
};

InvariantStringPiece::InvariantStringPiece(const UnicodeString &s) {
    int32_t  len = s.length();
    if (len+1 > buf.getCapacity()) {
        buf.resize(len+1);
    }
    
    s.extract(0, len, buf.getAlias(), len+1, US_INV);
    this->set(buf.getAlias(), len);
}







class UnicodeStringPiece: public StringPiece {
  public:
    UnicodeStringPiece(const UnicodeString &s);
    ~UnicodeStringPiece() {};
  private:
    MaybeStackArray<char, 20>  buf;
};

UnicodeStringPiece::UnicodeStringPiece(const UnicodeString &s) {
    int32_t  len = s.length();
    int32_t  capacity = buf.getCapacity();
    int32_t requiredCapacity = s.extract(0, len, buf.getAlias(), capacity) + 1;
    if (capacity < requiredCapacity) {
        buf.resize(requiredCapacity);
        capacity = requiredCapacity;
        s.extract(0, len, buf.getAlias(), capacity);
    }
    this->set(buf.getAlias(), requiredCapacity - 1);
}











static const char *formattableType(Formattable::Type typ) {
    static const char *types[] = {"kDate",
                                  "kDouble",
                                  "kLong",
                                  "kString",
                                  "kArray",
                                  "kInt64",
                                  "kObject"
                                  };
    if (typ<0 || typ>Formattable::kObject) {
        return "Unknown";
    }
    return types[typ];
}

const char *
DecimalFormatTest::getPath(char *buffer, const char *filename) {
    UErrorCode status=U_ZERO_ERROR;
    const char *testDataDirectory = IntlTest::getSourceTestData(status);
    DF_CHECK_STATUS;

    strcpy(buffer, testDataDirectory);
    strcat(buffer, filename);
    return buffer;
}

void DecimalFormatTest::DataDrivenTests() {
    char tdd[2048];
    const char *srcPath;
    UErrorCode  status  = U_ZERO_ERROR;
    int32_t     lineNum = 0;

    
    
    
    srcPath=getPath(tdd, "dcfmtest.txt");
    if(srcPath==NULL) {
        return; 
    }

    int32_t    len;
    UChar *testData = ReadAndConvertFile(srcPath, len, status);
    if (U_FAILURE(status)) {
        return; 
    }

    
    
    
    UnicodeString testString(FALSE, testData, len);

    RegexMatcher    parseLineMat(UnicodeString(
            "(?i)\\s*parse\\s+"
            "\"([^\"]*)\"\\s+"           
            "([ild])\\s+"                
            "\"([^\"]*)\"\\s+"           
            "\\s*(?:#.*)?"),             
         0, status);

    RegexMatcher    formatLineMat(UnicodeString(
            "(?i)\\s*format\\s+"
            "(\\S+)\\s+"                 
            "(ceiling|floor|down|up|halfeven|halfdown|halfup|default|unnecessary)\\s+"  
            "\"([^\"]*)\"\\s+"           
            "\"([^\"]*)\""               
            "\\s*(?:#.*)?"),             
         0, status);

    RegexMatcher    commentMat    (UNICODE_STRING_SIMPLE("\\s*(#.*)?$"), 0, status);
    RegexMatcher    lineMat(UNICODE_STRING_SIMPLE("(?m)^(.*?)$"), testString, 0, status);

    if (U_FAILURE(status)){
        dataerrln("Construct RegexMatcher() error.");
        delete [] testData;
        return;
    }

    
    
    
    while (lineMat.find()) {
        lineNum++;
        if (U_FAILURE(status)) {
            dataerrln("File dcfmtest.txt, line %d: ICU Error \"%s\"", lineNum, u_errorName(status));
        }

        status = U_ZERO_ERROR;
        UnicodeString testLine = lineMat.group(1, status);
        
        if (testLine.length() == 0) {
            continue;
        }

        
        
        
        

        commentMat.reset(testLine);
        if (commentMat.lookingAt(status)) {
            
            continue;
        }


        
        
        
        parseLineMat.reset(testLine);
        if (parseLineMat.lookingAt(status)) {
            execParseTest(lineNum,
                          parseLineMat.group(1, status),    
                          parseLineMat.group(2, status),    
                          parseLineMat.group(3, status),    
                          status
                          );
            continue;
        }

        
        
        
        formatLineMat.reset(testLine);
        if (formatLineMat.lookingAt(status)) {
            execFormatTest(lineNum,
                           formatLineMat.group(1, status),    
                           formatLineMat.group(2, status),    
                           formatLineMat.group(3, status),    
                           formatLineMat.group(4, status),    
                           kFormattable,
                           status);

            execFormatTest(lineNum,
                           formatLineMat.group(1, status),    
                           formatLineMat.group(2, status),    
                           formatLineMat.group(3, status),    
                           formatLineMat.group(4, status),    
                           kStringPiece,
                           status);
            continue;
        }

        
        
        
        errln("Badly formed test case at line %d.\n%s\n", 
             lineNum, UnicodeStringPiece(testLine).data());

    }

    delete [] testData;
}



void DecimalFormatTest::execParseTest(int32_t lineNum,
                                     const UnicodeString &inputText,
                                     const UnicodeString &expectedType,
                                     const UnicodeString &expectedDecimal,
                                     UErrorCode &status) {
    
    if (U_FAILURE(status)) {
        return;
    }

    DecimalFormatSymbols symbols(Locale::getUS(), status);
    UnicodeString pattern = UNICODE_STRING_SIMPLE("####");
    DecimalFormat format(pattern, symbols, status);
    Formattable   result;
    if (U_FAILURE(status)) {
        dataerrln("file dcfmtest.txt, line %d: %s error creating the formatter.",
            lineNum, u_errorName(status));
        return;
    }

    ParsePosition pos;
    int32_t expectedParseEndPosition = inputText.length();

    format.parse(inputText, result, pos);

    if (expectedParseEndPosition != pos.getIndex()) {
        errln("file dcfmtest.txt, line %d: Expected parse position afeter parsing: %d.  "
              "Actual parse position: %d", expectedParseEndPosition, pos.getIndex());
        return;
    }

    char   expectedTypeC[2];
    expectedType.extract(0, 1, expectedTypeC, 2, US_INV);
    Formattable::Type expectType = Formattable::kDate;
    switch (expectedTypeC[0]) {
      case 'd': expectType = Formattable::kDouble; break;
      case 'i': expectType = Formattable::kLong;   break;
      case 'l': expectType = Formattable::kInt64;  break;
      default:
          errln("file dcfmtest.tx, line %d: unrecongized expected type \"%s\"",
              lineNum, InvariantStringPiece(expectedType).data());
          return;
    }
    if (result.getType() != expectType) {
        errln("file dcfmtest.txt, line %d: expectedParseType(%s) != actual parseType(%s)",
             lineNum, formattableType(expectType), formattableType(result.getType()));
        return;
    }

    StringPiece decimalResult = result.getDecimalNumber(status);
    if (U_FAILURE(status)) {
        errln("File %s, line %d: error %s.  Line in file dcfmtest.txt:  %d:",
            __FILE__, __LINE__, u_errorName(status), lineNum);
        return;
    }

    InvariantStringPiece expectedResults(expectedDecimal);
    if (decimalResult != expectedResults) {
        errln("file dcfmtest.txt, line %d: expected \"%s\", got \"%s\"",
            lineNum, expectedResults.data(), decimalResult.data());
    }
    
    return;
}


void DecimalFormatTest::execFormatTest(int32_t lineNum,
                           const UnicodeString &pattern,     
                           const UnicodeString &round,       
                           const UnicodeString &input,       
                           const UnicodeString &expected,    
                           EFormatInputType inType,          
                           UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }

    DecimalFormatSymbols symbols(Locale::getUS(), status);
    
    DecimalFormat fmtr(pattern, symbols, status);
    if (U_FAILURE(status)) {
        dataerrln("file dcfmtest.txt, line %d: %s error creating the formatter.",
            lineNum, u_errorName(status));
        return;
    }
    if (round=="ceiling") {
        fmtr.setRoundingMode(DecimalFormat::kRoundCeiling);
    } else if (round=="floor") {
        fmtr.setRoundingMode(DecimalFormat::kRoundFloor);
    } else if (round=="down") {
        fmtr.setRoundingMode(DecimalFormat::kRoundDown);
    } else if (round=="up") {
        fmtr.setRoundingMode(DecimalFormat::kRoundUp);
    } else if (round=="halfeven") {
        fmtr.setRoundingMode(DecimalFormat::kRoundHalfEven);
    } else if (round=="halfdown") {
        fmtr.setRoundingMode(DecimalFormat::kRoundHalfDown);
    } else if (round=="halfup") {
        fmtr.setRoundingMode(DecimalFormat::kRoundHalfUp);
    } else if (round=="default") {
        
    } else if (round=="unnecessary") {
        fmtr.setRoundingMode(DecimalFormat::kRoundUnnecessary);
    } else {
        fmtr.setRoundingMode(DecimalFormat::kRoundFloor);
        errln("file dcfmtest.txt, line %d: Bad rounding mode \"%s\"",
                lineNum, UnicodeStringPiece(round).data());
    }

    const char *typeStr = "Unknown";
    UnicodeString result;
    UnicodeStringPiece spInput(input);

    switch (inType) {
    case kFormattable:
        {
            typeStr = "Formattable";
            Formattable fmtbl;
            fmtbl.setDecimalNumber(spInput, status);
            fmtr.format(fmtbl, result, NULL, status);
        }
        break;
    case kStringPiece:
        typeStr = "StringPiece";
        fmtr.format(spInput, result, NULL, status);
        break;
    }

    if ((status == U_FORMAT_INEXACT_ERROR) && (result == "") && (expected == "Inexact")) {
        
        status = U_ZERO_ERROR;
        return;
    }

    if (U_FAILURE(status)) {
        errln("[%s] file dcfmtest.txt, line %d: format() returned %s.",
            typeStr, lineNum, u_errorName(status));
        status = U_ZERO_ERROR;
        return;
    }
    
    if (result != expected) {
        errln("[%s] file dcfmtest.txt, line %d: expected \"%s\", got \"%s\"",
            typeStr, lineNum, UnicodeStringPiece(expected).data(), UnicodeStringPiece(result).data());
    }
}










UChar *DecimalFormatTest::ReadAndConvertFile(const char *fileName, int32_t &ulen,
                                     UErrorCode &status) {
    UChar       *retPtr  = NULL;
    char        *fileBuf = NULL;
    const char  *fileBufNoBOM = NULL;
    FILE        *f       = NULL;

    ulen = 0;
    if (U_FAILURE(status)) {
        return retPtr;
    }

    
    
    
    f = fopen(fileName, "rb");
    if (f == 0) {
        dataerrln("Error opening test data file %s\n", fileName);
        status = U_FILE_ACCESS_ERROR;
        return NULL;
    }
    
    
    
    int32_t            fileSize;
    int32_t            amtRead;
    int32_t            amtReadNoBOM;

    fseek( f, 0, SEEK_END);
    fileSize = ftell(f);
    fileBuf = new char[fileSize];
    fseek(f, 0, SEEK_SET);
    amtRead = fread(fileBuf, 1, fileSize, f);
    if (amtRead != fileSize || fileSize <= 0) {
        errln("Error reading test data file.");
        goto cleanUpAndReturn;
    }

    
    
    
    
    
    
    
    fileBufNoBOM = fileBuf + 3;
    amtReadNoBOM = amtRead - 3;
    if (fileSize<3 || uprv_strncmp(fileBuf, "\xEF\xBB\xBF", 3) != 0) {
        
        errln("Test data file %s is missing its BOM", fileName);
        fileBufNoBOM = fileBuf;
        amtReadNoBOM = amtRead;
    }

    
    
    
    
    u_strFromUTF8(NULL, 0, &ulen, fileBufNoBOM, amtReadNoBOM, &status);

    
    
    
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        
        status = U_ZERO_ERROR;
        retPtr = new UChar[ulen+1];
        u_strFromUTF8(retPtr, ulen+1, NULL, fileBufNoBOM, amtReadNoBOM, &status);
    }

cleanUpAndReturn:
    fclose(f);
    delete[] fileBuf;
    if (U_FAILURE(status)) {
        errln("ICU Error \"%s\"\n", u_errorName(status));
        delete retPtr;
        retPtr = NULL;
    };
    return retPtr;
}

#endif  

