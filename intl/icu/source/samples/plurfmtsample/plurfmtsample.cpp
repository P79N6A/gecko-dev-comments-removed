






#include <iostream>
#include "unicode/plurfmt.h"
#include "unicode/msgfmt.h"
#include "unicode/ustdio.h"


using namespace std;
static void PluralFormatExample() {
	  
	u_printf("=============================================================================\n");
	u_printf(" PluralFormatExample()\n");
    u_printf("\n");
    u_printf(" Use PluralFormat and Messageformat to get Plural Form for languages below:\n");
    u_printf(" English, Slovenian\n");
    u_printf("=============================================================================\n");
	
	
	UErrorCode status =U_ZERO_ERROR; 
	Locale locEn = Locale("en");
    Locale locSl = Locale("sl");

    UnicodeString patEn = UnicodeString("one{dog} other{dogs}");                      
    UnicodeString patSl = UnicodeString("one{pes} two{psa} few{psi} other{psov}");    

    
    PluralFormat plfmtEn = PluralFormat(locEn, patEn,status);
    PluralFormat plfmtSl = PluralFormat(locSl, patSl,status);
    
    MessageFormat* msgfmtEn =  new MessageFormat("{0,number} {1}", locEn,status);
    MessageFormat* msgfmtSl =  new MessageFormat("{0,number} {1}", locSl,status);

	int numbers[] = {0, 1, 2, 3, 4, 5, 10, 100, 101, 102};
	u_printf("Output by using PluralFormat and MessageFormat API\n");
    u_printf("%-16s%-16s%-16s\n","Number", "English","Slovenian");
 
    
    for (int i=0;i<sizeof(numbers)/sizeof(int);i++) {
	      UnicodeString msgEn,msgSl;
		  FieldPosition fpos = 0;
		  Formattable argEn[]={Formattable(numbers[i]), Formattable(plfmtEn.format(numbers[i],status))};
		  Formattable argSl[]={Formattable(numbers[i]), Formattable(plfmtSl.format(numbers[i],status))};
		  msgfmtEn->format(argEn,2,msgEn,fpos,status);
		  msgfmtSl->format(argSl,2,msgSl,fpos,status);
  		  u_printf("%-16d%-16S%-16S\n", numbers[i], msgEn.getTerminatedBuffer(),msgSl.getTerminatedBuffer());
      }

     u_printf("\n");

      
      UnicodeString msgPatEn = "{0,plural, one{# dog} other{# dogs}}";
      UnicodeString msgPatSl = "{0,plural, one{# pes} two{# psa} few{# psi} other{# psov}}";
 
	  MessageFormat* altMsgfmtEn = new MessageFormat(msgPatEn, locEn,status);
      MessageFormat* altMsgfmtSl = new MessageFormat(msgPatSl, locSl,status);
      u_printf("Same Output by using MessageFormat API only\n");
      u_printf("%-16s%-16s%-16s\n","Number", "English","Slovenian");
      for (int i=0;i<sizeof(numbers)/sizeof(int);i++) {
          UnicodeString msgEn,msgSl;
		  Formattable arg[] = {numbers[i]};
		  FieldPosition fPos =0;
		  altMsgfmtEn->format(arg, 1, msgEn, fPos, status);
          altMsgfmtSl->format(arg, 1, msgSl, fPos,status);
          u_printf("%-16d%-16S%-16S\n", numbers[i], msgEn.getTerminatedBuffer(), msgSl.getTerminatedBuffer());
      }

 	delete msgfmtEn;
	delete msgfmtSl;
	delete altMsgfmtEn;
	delete altMsgfmtSl;
	

	  














}
int main (int argc, char* argv[])
{
	PluralFormatExample();
	return 0;
}