





#include <iostream>
#include "unicode/smpdtfmt.h"
#include "unicode/dtptngen.h"
#include "unicode/ustdio.h"


using namespace std;

static void getBestPatternExample() {
	    
		u_printf("========================================================================\n");
		u_printf(" getBestPatternExample()\n");
        u_printf("\n");
        u_printf(" Use DateTimePatternGenerator to create customized date/time pattern:\n");
        u_printf(" yQQQQ,yMMMM, MMMMd, hhmm, jjmm per locale\n");
        u_printf("========================================================================\n");
		
	UnicodeString skeletons [] = {
		UnicodeString("yQQQQ"), 
        UnicodeString("yMMMM"), 
        UnicodeString("MMMMd"), 
        UnicodeString("hhmm"),  
        UnicodeString("jjmm"), 
		0,
	};

	Locale locales[] = {
		Locale ("en_US"),
		Locale ("fr_FR"),
		Locale ("zh_CN"),
	};
	
	const char* filename = "sample.txt";
	
	UFILE* f = u_fopen(filename, "w", NULL,"UTF-8");
	UnicodeString dateReturned;
	UErrorCode status =U_ZERO_ERROR;
	Calendar *cal = Calendar::createInstance(status);
	cal->set (1999,9,13,23,58,59);
	UDate date = cal->getTime(status);
	u_fprintf(f, "%-20S%-20S%-20S%-20S\n", UnicodeString("Skeleton").getTerminatedBuffer(),UnicodeString("en_US").getTerminatedBuffer(),UnicodeString("fr_FR").getTerminatedBuffer(),UnicodeString("zh_CN").getTerminatedBuffer());
	for (int i=0;skeletons[i]!=NULL;i++) {
		u_fprintf(f, "%-20S",skeletons[i].getTerminatedBuffer());
		for (int j=0;j<sizeof(locales)/sizeof(locales[0]);j++) {
			
			DateTimePatternGenerator *dtfg= DateTimePatternGenerator::createInstance(locales[j],status);
			
			UnicodeString pattern = dtfg->getBestPattern(skeletons[i],status);
			
			SimpleDateFormat *sdf = new SimpleDateFormat(pattern,locales[j],status);
			dateReturned.remove();
			
			sdf->format(date,dateReturned,status);
			
			u_fprintf(f, "%-20S", dateReturned.getTerminatedBuffer());
			delete dtfg;
			delete sdf;
		} 
		u_fprintf(f,"\n");
	}
	
	u_fclose(f);
	delete cal;
	
}

static void addPatternExample() {
		
		u_printf("========================================================================\n");
        u_printf(" addPatternExample()\n");
		u_printf("\n");
        u_printf(" Use addPattern API to add new '. von' to existing pattern\n");
        u_printf("========================================================================\n");
		
		UErrorCode status =U_ZERO_ERROR;
		UnicodeString conflictingPattern,dateReturned, pattern;
		Locale locale=Locale::getFrance();
		Calendar *cal = Calendar::createInstance(status);
		cal->set (1999,9,13,23,58,59);
		UDate date = cal->getTime(status);
        
		DateTimePatternGenerator *dtfg= DateTimePatternGenerator::createInstance(locale,status);
		SimpleDateFormat *sdf = new SimpleDateFormat(dtfg->getBestPattern("MMMMddHmm",status),locale,status);
        
        dtfg->addPattern("dd'. von' MMMM", true, conflictingPattern,status);
        
        sdf->applyPattern(dtfg->getBestPattern("MMMMddHmm",status));
		dateReturned = sdf->format(date, dateReturned, status);
		pattern =sdf->toPattern(pattern);
		u_printf("%s\n", "New Pattern for FRENCH: ");
      	u_printf("%S\n", pattern.getTerminatedBuffer());
		u_printf("%s\n", "Date Time in new Pattern: ");
		u_printf("%S\n", dateReturned.getTerminatedBuffer());
		delete dtfg;
		delete sdf;
		delete cal;

		
        





 	}

static void replaceFieldTypesExample() {
		
       u_printf("========================================================================\n");
       u_printf(" replaceFieldTypeExample()\n");
       u_printf("\n");
       u_printf(" Use replaceFieldTypes API to replace zone 'zzzz' with 'vvvv'\n");
       u_printf("========================================================================\n");
	   
		UFILE *out = u_finit(stdout, NULL, "UTF-8");
		UErrorCode status =U_ZERO_ERROR;
		UnicodeString pattern,dateReturned;
		Locale locale =Locale::getFrance();
		Calendar *cal = Calendar::createInstance(status);
		cal->set (1999,9,13,23,58,59);
		UDate date = cal->getTime(status);
		TimeZone *zone = TimeZone::createTimeZone(UnicodeString("Europe/Paris"));
		DateTimePatternGenerator *dtfg = DateTimePatternGenerator::createInstance(locale,status);
	    SimpleDateFormat *sdf = new SimpleDateFormat("EEEE d MMMM y HH:mm:ss zzzz",locale,status);
		sdf->setTimeZone(*zone);
		pattern = sdf->toPattern(pattern);
		u_fprintf(out, "%S\n", UnicodeString("Pattern before replacement:").getTerminatedBuffer());
      	u_fprintf(out, "%S\n", pattern.getTerminatedBuffer());
		dateReturned.remove();
		dateReturned = sdf->format(date, dateReturned, status);
		u_fprintf(out, "%S\n", UnicodeString("Date/Time format in fr_FR:").getTerminatedBuffer());
		u_fprintf(out, "%S\n", dateReturned.getTerminatedBuffer());
        
		UnicodeString newPattern = dtfg->replaceFieldTypes(pattern, "vvvv", status);
		
		sdf->applyPattern(newPattern);
		dateReturned.remove();
		dateReturned = sdf->format(date, dateReturned, status);
		u_fprintf(out, "%S\n", UnicodeString("Pattern after replacement:").getTerminatedBuffer());
     	u_fprintf(out, "%S\n", newPattern.getTerminatedBuffer());
     	u_fprintf(out, "%S\n", UnicodeString("Date/Time format in fr_FR:").getTerminatedBuffer());
		u_fprintf(out, "%S\n", dateReturned.getTerminatedBuffer());
		delete sdf;
		delete dtfg;
		delete zone;
		delete cal;
		u_fclose(out);
	
    }

int main (int argc, char* argv[])
{
	getBestPatternExample();
	addPatternExample();
	replaceFieldTypesExample();
	return 0;
}
