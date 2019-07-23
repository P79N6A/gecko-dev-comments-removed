






































 
#ifndef _PSOBJ_H_
#define _PSOBJ_H_

#include "prtypes.h"  
#ifdef __cplusplus
#include "nsColor.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsString.h"

#include "nsCOMPtr.h"
#include "nsIPref.h"
#include "nsHashtable.h"
#include "nsIUnicodeEncoder.h"

#include "nsIDeviceContextSpecPS.h"
#include "nsIPersistentProperties2.h"
#include "nsTempfilePS.h"
#include "nsEPSObjectPS.h"

class nsIImage;
class nsIAtom;
#endif

#include <stdio.h>

#define N_FONTS 8
#define INCH_TO_PAGE(f) ((int) (.5 + (f)*720))

typedef int XP_Bool;

typedef void (*XL_CompletionRoutine)(void*);

typedef struct page_breaks {
    int32 y_top;
    int32 y_break;
} PageBreaks;

#ifdef __cplusplus
typedef struct PS_LangGroupInfo_ {
  nsIUnicodeEncoder *mEncoder;
  nsHashtable       *mU2Ntable;
} PS_LangGroupInfo;
#endif

typedef struct LineRecord_struct LineRecord;




struct PrintInfo_ {
  const char *doc_title; 

#ifdef LATER
  THIS IS GOING TO BE DELETED XXXXX
  float	scale;		
  int32	pre_start;	
  int32	pre_end;	
  XP_List	*interesting;	
  XP_Bool	in_pre;		
#endif

  
  char *line;		          
  XP_Bool in_table;	      
  XP_Bool first_line_p;		

  int table_top,table_bottom;
  LineRecord *saved_lines;	
  int last_y;		
};

typedef struct PrintInfo_ PrintInfo;




struct PrintSetup_ {
  PRInt32 width;                
  PRInt32 height;               
  
  const char* header;
  const char* footer;
  const char* paper_name;

  int *sizes;
  XP_Bool reverse;              
  XP_Bool color;                
  XP_Bool deep_color;		        
  XP_Bool landscape;            
  XP_Bool underline;            
  XP_Bool scale_images;         
  XP_Bool scale_pre;		        
  float rules;			            
  int n_up;                     
  int bigger;                   
  const char* prefix;           
  const char* eol;              
  const char* bullet;           

  struct URL_Struct_ *url;      
  XL_CompletionRoutine completion; 
  void* carg;                   
  int status;                   
  int num_copies;               
};

typedef struct PrintSetup_ PrintSetup;

struct PSContext_{

    char        *url;         
    char        * name;	      
    char        * title;		  
    PrintSetup	*prSetup;	    
    PrintInfo	  *prInfo;	    
};
typedef struct PSContext_ PSContext;

struct PSBoundingBox {          
  float llx, lly, urx, ury;
};

#ifdef __cplusplus
class nsPostScriptObj
{


public:
  nsPostScriptObj();
  ~nsPostScriptObj();
  
  
  



  nsresult Init( nsIDeviceContextSpecPS *aSpec );
  



  void begin_page();
  



  void end_page();

  



  nsresult end_document();

  





  void write_prolog(FILE *aHandle, PRBool aFTPenable = PR_FALSE);

  




  nsresult write_script(FILE *aHandle);

  






  void moveto(nscoord aX, nscoord aY);
  







  void lineto(nscoord aX, nscoord aY);
  



  void closepath();
  





  void arc(nscoord aWidth, nscoord aHeight,float aStartAngle,float aEndAngle);
  








  void box(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  






  void box_subtract(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  








  void line(nscoord aX1, nscoord aY1, nscoord aX2, nscoord aY2, nscoord aThick);
  



  void stroke();
  



  void fill();
  



  void graphics_save();
  



  void graphics_restore();

  

















  void draw_image(nsIImage *anImage,
      const nsRect& sRect, const nsRect& iRect, const nsRect& dRect);

  



  void begin_squished_text( float aSqeeze);
  



  void end_squished_text();
  



  void finalize_translation();
  




  void scale(float aX, float aY);
  





  void translate(nscoord aX, nscoord aY);
  



  void show(const char* aText, int aLen, const char *aAlign);
  



  void show(const PRUnichar* aText, int aLen, const char *aAlign, int aType);
  




  void show(const PRUnichar* aText, int aLen, const nsAFlatString& aCharList,
            PRUint16 aSubFontIdx);
  



  void clip();
  



  void eoclip(); 
  



  void newpath();
  



  void initclip();
  



  void clippath();
  



  void setcolor(nscolor aTheColor);
  



  void setscriptfont(PRInt16 aFontIndex,const nsString &aFamily,nscoord aHeight, PRUint8 aStyle, PRUint8 aVariant, PRUint16 aWeight, PRUint8 decorations);
  



  void setfont(const nsCString &aFontName, PRUint32 aHeight,
               PRInt32 aSubFont = -1);
  



  void comment(const char *aTheComment);
  



  void setlanggroup(nsIAtom* aLangGroup);
  



  void preshow(const PRUnichar* aText, int aLen);

  






  nsresult render_eps(const nsRect& aRect, nsEPSObjectPS &anEPS);
  
  void settitle(PRUnichar * aTitle);

  




  FILE * GetScriptHandle() { return mScriptFP; }

  









  void SetNumCopies(int aNumCopies);

  PRBool  GetUnixPrinterSetting(const nsCAutoString&, char**);
  PrintSetup            *mPrintSetup;
private:
  PSContext             *mPrintContext;
  PRUint16              mPageNumber;
  nsCOMPtr<nsIPersistentProperties> mPrinterProps;
  char                  *mTitle;
  nsTempfilePS          mTempfileFactory;
  nsCOMPtr<nsILocalFile> mDocScript;
  FILE                  *mScriptFP;


  



  void initialize_translation(PrintSetup* aPi);
  




  void initlanggroup(FILE *aHandle);

};

#endif 

#endif
