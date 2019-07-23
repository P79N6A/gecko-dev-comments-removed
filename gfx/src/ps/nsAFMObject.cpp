




































#include "nsAFMObject.h"
#include "Helvetica.h"
#include "Helvetica-Bold.h"
#include "Helvetica-BoldOblique.h"
#include "Helvetica-Oblique.h"
#include "Times-Roman.h"
#include "Times-Bold.h"
#include "Times-BoldItalic.h"
#include "Times-Italic.h"
#include "Courier.h"
#include "Courier-Bold.h"
#include "Courier-BoldOblique.h"
#include "Courier-Oblique.h"
#include "Symbol.h"
#include "nsReadableUtils.h"
#include "nsVoidArray.h"

struct SubstituteMap {
  const char *name;
  PRUint8    italic;
  PRBool     bold;
  PRInt16    index;
};

static SubstituteMap gSubstituteMap[] = {
  { "serif", 0,0,0},
  { "serif", 0,1,1},
  { "serif", 1,1,2},
  { "serif", 1,0,3},
  { "sans-serif", 0,0,4},
  { "sans-serif", 0,1,5},
  { "sans-serif", 1,1,6},
  { "sans-serif", 1,0,7},
  { "monospace", 0,0,8},
  { "monospace", 0,1,9},
  { "monospace", 1,1,10},
  { "monospace", 1,0,11},
};


static  const PRUint32 gNumSubstituteMap = sizeof(gSubstituteMap)/sizeof(SubstituteMap);

#define NS_IS_BOLD(weight)  ((weight) >= 401 ? 1 : 0)


DefFonts gSubstituteFonts[] = 
{
  {"Times-Roman","Times",400,0,&Times_RomanAFM,AFMTimes_RomanChars,-1},
  {"Times-Bold","Times",700,0,&Times_BoldAFM,AFMTimes_BoldChars,-1},
  {"Times-BoldItalic","Times",700,1,&Times_BoldItalicAFM,AFMTimes_BoldItalicChars,-1},
  {"Times-Italic","Times",400,1,&Times_ItalicAFM,AFMTimes_ItalicChars,-1},
  {"Helvetica","Helvetica",400,0,&HelveticaAFM,AFMHelveticaChars,-1},
  {"Helvetica-Bold","Helvetica",700,0,&Helvetica_BoldAFM,AFMHelvetica_BoldChars,-1},
  {"Helvetica-BoldOblique","Helvetica",700,2,&Helvetica_BoldObliqueAFM,AFMHelvetica_BoldObliqueChars,-1},
  {"Helvetica-Oblique","Helvetica",400,2,&Helvetica_ObliqueAFM,AFMHelvetica_ObliqueChars,-1},
  {"Courier","Courier",400,0,&CourierAFM,AFMCourierChars,-1},
  {"Courier-Bold","Courier",700,0,&Courier_BoldAFM,AFMCourier_BoldChars,-1},
  {"Courier-BoldOblique","Courier",700,2,&Courier_BoldObliqueAFM,AFMCourier_BoldObliqueChars,-1},
  {"Courier-Oblique","Courier",400,2,&Courier_ObliqueAFM,AFMCourier_ObliqueChars,-1},
  {"Symbol","Symbol",400,0,&SymbolAFM,AFMSymbolChars,-1}
};






static struct keyname_st
{
  const char *name;
  AFMKey key;
} keynames[] =
{
  {"Ascender", 			kAscender},
  {"Axes", 			kAxes},
  {"AxisLabel", 		kAxisLabel},
  {"AxisType", 			kAxisType},
  {"B",				kB},
  {"BlendAxisTypes", 		kBlendAxisTypes},
  {"BlendDesignMap", 		kBlendDesignMap},
  {"BlendDesignPositions", 	kBlendDesignPositions},
  {"C",				kC},
  {"CC", 			kCC},
  {"CH",			kCH},
  {"CapHeight", 		kCapHeight},
  {"CharWidth", 		kCharWidth},
  {"CharacterSet", 		kCharacterSet},
  {"Characters", 		kCharacters},
  {"Comment", 			kComment},
  {"Descendents", 		kDescendents},
  {"Descender", 		kDescender},
  {"EncodingScheme", 		kEncodingScheme},
  {"EndAxis", 			kEndAxis},
  {"EndCharMetrics", 		kEndCharMetrics},
  {"EndCompFontMetrics", 	kEndCompFontMetrics},
  {"EndComposites", 		kEndComposites},
  {"EndDescendent", 		kEndDescendent},
  {"EndDirection", 		kEndDirection},
  {"EndFontMetrics", 		kEndFontMetrics},
  {"EndKernData", 		kEndKernData},
  {"EndKernPairs",		kEndKernPairs},
  {"EndMaster", 		kEndMaster},
  {"EndMasterFontMetrics", 	kEndMasterFontMetrics},
  {"EndTrackKern", 		kEndTrackKern},
  {"EscChar", 			kEscChar},
  {"FamilyName", 		kFamilyName},
  {"FontBBox", 			kFontBBox},
  {"FontName", 			kFontName},
  {"FullName", 			kFullName},
  {"IsBaseFont", 		kIsBaseFont},
  {"IsFixedPitch", 		kIsFixedPitch},
  {"IsFixedV", 			kIsFixedV},
  {"ItalicAngle", 		kItalicAngle},
  {"KP", 			kKP},
  {"KPH", 			kKPH},
  {"KPX", 			kKPX},
  {"KPY", 			kKPY},
  {"L",				kL},
  {"MappingScheme", 		kMappingScheme},
  {"Masters", 			kMasters},
  {"MetricsSets", 		kMetricsSets},
  {"N",				kN},
  {"Notice", 			kNotice},
  {"PCC", 			kPCC},
  {"StartAxis", 		kStartAxis},
  {"StartCharMetrics", 		kStartCharMetrics},
  {"StartCompFontMetrics", 	kStartCompFontMetrics},
  {"StartComposites", 		kStartComposites},
  {"StartDescendent", 		kStartDescendent},
  {"StartDirection", 		kStartDirection},
  {"StartFontMetrics", 		kStartFontMetrics},
  {"StartKernData", 		kStartKernData},
  {"StartKernPairs",		kStartKernPairs},
  {"StartMaster", 		kStartMaster},
  {"StartMasterFontMetrics", 	kStartMasterFontMetrics},
  {"StartTrackKern", 		kStartTrackKern},
  {"TrackKern", 		kTrackKern},
  {"UnderlinePosition", 	kUnderlinePosition},
  {"UnderlineThickness", 	kUnderlineThickness},
  {"VV",			kVV},
  {"VVector", 			kVVector},
  {"Version", 			kVersion},
  {"W",				kW},
  {"W0",			kW0},
  {"W0X",			kW0X},
  {"W0Y",			kW0Y},
  {"W1",			kW1},
  {"W1X",			kW1X},
  {"W1Y",			kW1Y},
  {"WX",			kWX},
  {"WY",			kWY},
  {"Weight", 			kWeight},
  {"WeightVector", 		kWeightVector},
  {"XHeight", 			kXHeight},
  {"", (AFMKey)0},
};

#define ISSPACE(ch) ((ch)==' '||(ch)=='\n'||(ch)=='\r'||(ch)=='\t'||(ch)==';')












nsAFMObject :: nsAFMObject()
{
  mPSFontInfo = nsnull;
}




 
nsAFMObject :: ~nsAFMObject()
{

  if (mPSFontInfo){
    if(mPSFontInfo->mAFMCharMetrics){
      delete [] mPSFontInfo->mAFMCharMetrics;
    }
    delete mPSFontInfo;
  }

}






void
nsAFMObject :: Init(nscoord aFontHeight)
{
  
  mFontHeight = aFontHeight;
}





PRInt16
nsAFMObject::CheckBasicFonts(const nsFont &aFont,PRBool aPrimaryOnly)
{
PRInt16     ourfont = -1;
PRInt32     i,curIndex,score;
nsAutoString    psfontname;

  
  psfontname = aFont.name;
  
  
  for(i=0,curIndex=-1;i<NUM_AFM_FONTS;i++){
    gSubstituteFonts[i].mIndex = psfontname.RFind((const char*)gSubstituteFonts[i].mFamily,PR_TRUE);

    
    if((gSubstituteFonts[i].mIndex==0) || (!aPrimaryOnly && gSubstituteFonts[i].mIndex>=0)){
      
      score = abs(PRInt32(aFont.weight)-PRInt32(gSubstituteFonts[i].mWeight));
      score+= abs(PRInt32(aFont.style)-PRInt32(gSubstituteFonts[i].mStyle));
      if(score == 0){
        curIndex = i;
        break;
      }
      gSubstituteFonts[i].mIndex = score;
    }
  }
  
  
  score = 32000;
  if((PR_FALSE == aPrimaryOnly)&&(curIndex !=0)) {
    for(i=0;i<NUM_AFM_FONTS;i++){
      if((gSubstituteFonts[i].mIndex>0) && (gSubstituteFonts[i].mIndex<score)){
        score = gSubstituteFonts[i].mIndex;
        curIndex = i;
      }   
    }
  }


  if(curIndex>=0){
    mPSFontInfo = new AFMFontInformation;
    memset(mPSFontInfo,0,sizeof(AFMFontInformation));
    
    memcpy(mPSFontInfo,(gSubstituteFonts[curIndex].mFontInfo),sizeof(AFMFontInformation));
    mPSFontInfo->mAFMCharMetrics = new AFMscm[mPSFontInfo->mNumCharacters];
    memset(mPSFontInfo->mAFMCharMetrics,0,sizeof(AFMscm)*mPSFontInfo->mNumCharacters);
    memcpy(mPSFontInfo->mAFMCharMetrics,gSubstituteFonts[curIndex].mCharInfo,gSubstituteFonts[curIndex].mFontInfo->mNumCharacters*sizeof(AFMscm));
    ourfont = curIndex;
  }
  
  return ourfont;
}




static PRBool PR_CALLBACK
GenericFontEnumCallback(const nsString& aFamily, PRBool aGeneric, void* aData)
{
  nsVoidArray* array = NS_STATIC_CAST(nsVoidArray*, aData);
  char* name = ToNewCString(aFamily);
  if (name) {
    array->AppendElement(name);
    return PR_TRUE; 
  }
  return PR_FALSE;
}





PRInt16
nsAFMObject::CreateSubstituteFont(const nsFont &aFontName)
{
PRInt16     ourfont = 0;
PRUint32    i = gNumSubstituteMap;

  
  nsVoidArray fontNames;
  
  aFontName.EnumerateFamilies(GenericFontEnumCallback, &fontNames); 

  PRInt32 k;
  PRBool found = PR_FALSE;
  for (k=0;k<fontNames.Count() && !found;k++) {
    char * fontName = (char*)fontNames[k];
    for(i=0;i<gNumSubstituteMap;i++) {
      
      if(!nsCRT::strcasecmp(fontName, gSubstituteMap[i].name) && 
         (aFontName.style != NS_FONT_STYLE_NORMAL)== gSubstituteMap[i].italic &&
         NS_IS_BOLD(aFontName.weight) == gSubstituteMap[i].bold) {
        ourfont = gSubstituteMap[i].index;
        found = PR_TRUE;
        break;
      }
    }
  } 

  for (k=0;k<fontNames.Count();k++) {
    nsMemory::Free((char*)fontNames[k]);
  }

  
  if(i == gNumSubstituteMap){

#ifdef DEBUG
    printf(" NO FONT WAS FOUND Name[%s]\n", NS_LossyConvertUTF16toASCII(aFontName.name).get());
#endif
    if(aFontName.style == NS_FONT_STYLE_NORMAL){
      ourfont = NS_IS_BOLD(aFontName.weight) ? 1 : 0;
    } else {
      ourfont = NS_IS_BOLD(aFontName.weight) ? 2 : 3;
    }
  }

  mPSFontInfo = new AFMFontInformation;
  memset(mPSFontInfo,0,sizeof(AFMFontInformation));

  
  memcpy(mPSFontInfo,gSubstituteFonts[ourfont].mFontInfo,sizeof(AFMFontInformation));
  mPSFontInfo->mAFMCharMetrics = new AFMscm[mPSFontInfo->mNumCharacters];
  memset(mPSFontInfo->mAFMCharMetrics,0,sizeof(AFMscm)*mPSFontInfo->mNumCharacters);
  memcpy(mPSFontInfo->mAFMCharMetrics,gSubstituteFonts[ourfont].mCharInfo,Times_RomanAFM.mNumCharacters*sizeof(AFMscm));
  return ourfont;
}





PRBool
nsAFMObject::AFM_ReadFile(const nsFont &aFontName)
{
PRBool  done=PR_FALSE;
PRBool  success = PR_FALSE;
PRBool  bvalue;
AFMKey  key;
double  value;
PRInt32 ivalue;
char* AFMFileName= ToNewUTF8String(aFontName.name); 

  if(nsnull == AFMFileName) 
    return (success);

    if((0==strcmp(AFMFileName,"..")) || (0==strcmp(AFMFileName,"."))) {
      Recycle(AFMFileName);
      return (success);
    }

   
  mAFMFile = fopen((const char *)AFMFileName,"r");
  Recycle(AFMFileName);

  if(nsnull != mAFMFile) {
    
    mPSFontInfo = new AFMFontInformation;
    memset(mPSFontInfo,0,sizeof(AFMFontInformation));

    
    GetKey(&key);
    if(key == kStartFontMetrics){
      GetAFMNumber(&mPSFontInfo->mFontVersion);

      while(!done){
        GetKey(&key);
        switch (key){
          case kComment:
            GetLine();
            break;
          case kStartFontMetrics:
            GetAFMNumber(&mPSFontInfo->mFontVersion);
            break;
          case kEndFontMetrics:
            done = PR_TRUE;
            break;
          case kStartCompFontMetrics:
          case kEndCompFontMetrics:
          case kStartMasterFontMetrics:
          case kEndMasterFontMetrics:
            break;
          case kFontName:
            mPSFontInfo->mFontName = GetAFMString();
            break;
          case kFullName:
            mPSFontInfo->mFullName = GetAFMString();
            break;
          case kFamilyName:
            mPSFontInfo->mFamilyName = GetAFMString();
            break;
          case kWeight:
            mPSFontInfo->mWeight = GetAFMString();
            break;
          case kFontBBox:
            GetAFMNumber(&mPSFontInfo->mFontBBox_llx);
            GetAFMNumber(&mPSFontInfo->mFontBBox_lly);
            GetAFMNumber(&mPSFontInfo->mFontBBox_urx);
            GetAFMNumber(&mPSFontInfo->mFontBBox_ury);
            break;
          case kVersion:
            mPSFontInfo->mVersion = GetAFMString();
            break;
	        case kNotice:
	          mPSFontInfo->mNotice = GetAFMString();
            
            delete [] mPSFontInfo->mNotice;
            mPSFontInfo->mNotice = 0;
	          break;
	        case kEncodingScheme:
	          mPSFontInfo->mEncodingScheme = GetAFMString();
	          break;
	        case kMappingScheme:
	          GetAFMInt(&mPSFontInfo->mMappingScheme);
	          break;
	        case kEscChar:
	          GetAFMInt(&mPSFontInfo->mEscChar);
	          break;
	        case kCharacterSet:
	          mPSFontInfo->mCharacterSet = GetAFMString();
	          break;
	        case kCharacters:
	          GetAFMInt(&mPSFontInfo->mCharacters);
	          break;
	        case kIsBaseFont:
	          GetAFMBool (&mPSFontInfo->mIsBaseFont);
	          break;
	        case kVVector:
	          GetAFMNumber(&mPSFontInfo->mVVector_0);
	          GetAFMNumber(&mPSFontInfo->mVVector_1);
	          break;
	        case kIsFixedV:
	          GetAFMBool (&mPSFontInfo->mIsFixedV);
	          break;
	        case kCapHeight:
	          GetAFMNumber(&mPSFontInfo->mCapHeight);
	          break;
	        case kXHeight:
	          GetAFMNumber(&mPSFontInfo->mXHeight);
	          break;
	        case kAscender:
	          GetAFMNumber(&mPSFontInfo->mAscender);
	          break;
	        case kDescender:
	          GetAFMNumber(&mPSFontInfo->mDescender);
	          break;
	        case kStartDirection:
	          GetAFMInt(&ivalue);
	          break;
	        case kUnderlinePosition:
	          GetAFMNumber(&mPSFontInfo->mUnderlinePosition);
	          break;
	        case kUnderlineThickness:
	          GetAFMNumber(&mPSFontInfo->mUnderlineThickness);
	          break;
	        case kItalicAngle:
	          GetAFMNumber(&value);
	          break;
	        case kCharWidth:
	          GetAFMNumber(&value);   
	          GetAFMNumber(&value);   
	          break;
	        case kIsFixedPitch:
	          GetAFMBool (&bvalue);
	          break;
	        case kEndDirection:
	          break;
	        case kStartCharMetrics:
	          GetAFMInt(&mPSFontInfo->mNumCharacters);     
            mPSFontInfo->mAFMCharMetrics = new AFMscm[mPSFontInfo->mNumCharacters];
            memset(mPSFontInfo->mAFMCharMetrics,0,sizeof(AFMscm)*mPSFontInfo->mNumCharacters);
	          ReadCharMetrics (mPSFontInfo,mPSFontInfo->mNumCharacters);
	          break;
	        case kStartKernData:
	          break;
	        case kStartKernPairs:
            break;
          default:
            break;
        }
      }
    }
  fclose(mAFMFile);
  success = PR_TRUE;
  } else {
  
  
 
  
  
  }

  return(success);
}





void

nsAFMObject::GetKey(AFMKey *aKey)
{
PRInt32   key,len;

  while(1){
    len = GetToken(); 
    if(len>0) {
      key = MatchKey(mToken);
      if(key >=0){
        *aKey = (AFMKey)key;
        return;
      }

    GetLine(); 
    }
  }
}





PRInt32
nsAFMObject::MatchKey(char *aKey)
{
PRInt32 lower = 0;
PRInt32 upper = NUM_KEYS;
PRInt32 midpoint,cmpvalue;
PRBool  found = PR_FALSE;

  while((upper >=lower) && !found) {
    midpoint = (lower+upper)/2;
    if(keynames[midpoint].name == nsnull) {
      break;
    }
    cmpvalue = strcmp(aKey,keynames[midpoint].name);
    if(cmpvalue == 0){
      found = PR_TRUE;
    }else{
     if (cmpvalue <0){
        upper = midpoint-1;
      }else{
        lower = midpoint+1;
      }
    }
  }

  if(found)
    return keynames[midpoint].key;
  else
    return -1;
}





PRInt32
nsAFMObject::GetToken()
{
PRInt32   ch;
PRInt32   i;
PRInt32   len;

  
  while((ch=getc(mAFMFile)) != EOF) {
    if(!ISSPACE(ch))
      break;
  }

  if(ch == EOF)
    return 0;

  ungetc(ch,mAFMFile);

  
  len = (PRInt32)sizeof(mToken);
  for(i=0,ch=getc(mAFMFile);i<len && ch!=EOF && !ISSPACE(ch);i++,ch=getc(mAFMFile)){
      mToken[i] = ch;
  }

  
  if(((PRUint32)i)>=sizeof(mToken))
    return 0;

  mToken[i] = '\0';
  return i;
}






PRInt32 
nsAFMObject::GetLine()
{
PRInt32 i, ch;

  
  while ((ch = getc (mAFMFile)) != EOF){
      if (!ISSPACE (ch))
        break;
  }

  if (ch == EOF)
    return 0;

  ungetc (ch, mAFMFile);

  
  for (i = 0, ch = getc (mAFMFile);((PRUint32)i) < sizeof (mToken) - 1 && ch != EOF && ch != '\n';i++, ch = getc (mAFMFile)){
    mToken[i] = ch;
  }

  if (((PRUint32)i) >= sizeof (mToken) - 1){
    
  }

  
  for (i--; i >= 0 && ISSPACE (mToken[i]); i--)
    ;
  i++;

  mToken[i] = '\0';

  return i;
}





void
nsAFMObject::GetAFMBool (PRBool *aBool)
{

  GetToken();
  if (strcmp (mToken, "true") == 0) {
	  *aBool = PR_TRUE;
  }else if(strcmp (mToken, "false")){
    *aBool = PR_FALSE;
  }else {
    *aBool = PR_FALSE;
  }
}





void
nsAFMObject::ReadCharMetrics (AFMFontInformation *aFontInfo,PRInt32 aNumCharacters)
{
PRInt32 i = 0,ivalue,first=1;
AFMscm  *cm = nsnull;
AFMKey  key;
PRBool  done = PR_FALSE;
double  notyet;
char    *name;

  while (done!=PR_TRUE && i<aNumCharacters){
    GetKey (&key);
    switch (key){
      case kC:
        if (first){
          first = 0;
        }else{
          i++;
        }
        if (i >= aNumCharacters){
          done = PR_TRUE;
          
        }

        cm = &(aFontInfo->mAFMCharMetrics[i]);
        
        GetAFMInt(&ivalue);          
        cm->mCharacter_Code = ivalue;
        
          
        break;
      case kCH:
        break;
      case kWX:
      case kW0X:
        GetAFMNumber(&(cm->mW0x));
        cm->mW0y = 0.0;
        break;
      case kW1X:
        GetAFMNumber(&(cm->mW1x));
        cm->mW1y = 0.0;
        break;
      case kWY:
      case kW0Y:
        GetAFMNumber(&(cm->mW0y));
        cm->mW0x = 0.0;
        break;
      case kW1Y:
        GetAFMNumber(&(cm->mW1y));
        cm->mW1x = 0.0;
        break;
      case kW:
      case kW0:
        GetAFMNumber(&(cm->mW0x));
        GetAFMNumber(&(cm->mW0y));
        break;

      case kW1:
        GetAFMNumber(&(cm->mW1x));
        GetAFMNumber(&(cm->mW1y));
        break;
      case kVV:
        
        
        GetAFMNumber(&notyet);
        GetAFMNumber(&notyet);
        break;
      case kN:
        
        name = GetAFMName();
        delete [] name;
        break;

      case kB:
        GetAFMNumber(&(cm->mLlx));
        GetAFMNumber(&(cm->mLly));
        GetAFMNumber(&(cm->mUrx));
        GetAFMNumber(&(cm->mUry));
        break;
      case kL:
        
        GetLine ();
      break;

      case kEndCharMetrics:
        
        done = PR_TRUE;
        break;
      default:
        break;
    }
  }
}






char*
nsAFMObject::GetAFMString (void) 
{
PRInt32 len;
char    *thestring;

  GetLine();
  len = strlen(mToken);
  thestring = new char[len+1];
  strcpy(thestring,mToken);
  return(thestring);
}





char*
nsAFMObject::GetAFMName (void) 
{
PRInt32 len;
char    *thestring;

  GetToken();
  len = strlen(mToken);
  thestring = new char[len+1];
  strcpy(thestring,mToken);
  return(thestring);
}








void
nsAFMObject :: GetStringWidth(const char *aString,nscoord& aWidth,nscoord aLength)
{
char    *cptr;
PRInt32 i,idx,fwidth;
float   totallen=0.0f;

  
  aWidth = 0;
  cptr = (char*) aString;
  for(i=0;i<aLength;i++,cptr++){
    idx = *cptr-32;
    fwidth = (PRInt32)(mPSFontInfo->mAFMCharMetrics[idx].mW0x);
    totallen += fwidth;
  }

  
  
  aWidth = NSToCoordRound((totallen * mFontHeight)/1000.0f);
}









void
nsAFMObject :: GetStringWidth(const PRUnichar *aString,nscoord& aWidth,nscoord aLength)
{
PRUint8   asciichar;
PRUnichar *cptr;
PRInt32   i ,fwidth,idx;
float     totallen=0.0f;

 
 aWidth = 0;
 cptr = (PRUnichar*)aString;

  for(i=0;i<aLength;i++,cptr++){
    fwidth = 0;
    if (*cptr & 0xff00)
    {
      if (0x0400 == (*cptr & 0xff00)) { 
        fwidth = 600;
      } else {
        fwidth = 1056;
      }
    } else {
      
      asciichar = (*cptr)&0x00ff;
      idx = asciichar-32;
      if(idx >= 0 )
        fwidth = (PRInt32)(mPSFontInfo->mAFMCharMetrics[idx].mW0x);
      else if (*cptr  == 0x0020) 
        fwidth = 1056;
    }

    totallen += fwidth;
  }

  
  
  aWidth = NSToCoordRound((totallen * mFontHeight)/1000.0f);
}



#define CORRECTSTRING(d)  (d?d:"")
#define BOOLOUT(B)        (mPSFontInfo->mIsBaseFont==PR_TRUE?"PR_TRUE":"PR_FALSE")





void    
nsAFMObject :: WriteFontHeaderInformation(FILE *aOutFile)
{

  
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mFontVersion);
  fprintf(aOutFile,"\"%s\",\n",CORRECTSTRING(mPSFontInfo->mFontName));
  fprintf(aOutFile,"\"%s\",\n",CORRECTSTRING(mPSFontInfo->mFullName));
  fprintf(aOutFile,"\"%s\",\n",CORRECTSTRING(mPSFontInfo->mFamilyName));
  fprintf(aOutFile,"\"%s\",\n",CORRECTSTRING(mPSFontInfo->mWeight));
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mFontBBox_llx);
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mFontBBox_lly);
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mFontBBox_urx);
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mFontBBox_ury);
  fprintf(aOutFile,"\"%s\",\n",CORRECTSTRING(mPSFontInfo->mVersion));
  fprintf(aOutFile,"\"%s\",\n",CORRECTSTRING(mPSFontInfo->mNotice));
  fprintf(aOutFile,"\"%s\",\n",CORRECTSTRING(mPSFontInfo->mEncodingScheme));
  fprintf(aOutFile,"%d,\n",mPSFontInfo->mMappingScheme);
  fprintf(aOutFile,"%d,\n",mPSFontInfo->mEscChar);
  fprintf(aOutFile,"\"%s\",\n", CORRECTSTRING(mPSFontInfo->mCharacterSet));
  fprintf(aOutFile,"%d,\n",mPSFontInfo->mCharacters);
  fprintf(aOutFile,"%s,\n",BOOLOUT(mPSFontInfo->mIsBaseFont));
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mVVector_0);
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mVVector_1);
  fprintf(aOutFile,"%s,\n",BOOLOUT(mPSFontInfo->mIsFixedV));
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mCapHeight);
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mXHeight);
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mAscender);
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mDescender);
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mUnderlinePosition);
  fprintf(aOutFile,"%f,\n",mPSFontInfo->mUnderlineThickness);
  fprintf(aOutFile,"%d\n",mPSFontInfo->mNumCharacters);
}





void    
nsAFMObject :: WriteFontCharInformation(FILE *aOutFile)
{
PRInt32 i;


  
  for(i=0;i<mPSFontInfo->mNumCharacters;i++) {
    fprintf(aOutFile,"{\n");
    fprintf(aOutFile,"%d, \n",mPSFontInfo->mAFMCharMetrics[i].mCharacter_Code);
    fprintf(aOutFile,"%f, \n",mPSFontInfo->mAFMCharMetrics[i].mW0x);
    fprintf(aOutFile,"%f, \n",mPSFontInfo->mAFMCharMetrics[i].mW0y);
    fprintf(aOutFile,"%f, \n",mPSFontInfo->mAFMCharMetrics[i].mW1x);
    fprintf(aOutFile,"%f, \n",mPSFontInfo->mAFMCharMetrics[i].mW1y);
    
    
    
    fprintf(aOutFile,"%f, \n",mPSFontInfo->mAFMCharMetrics[i].mLlx);
    fprintf(aOutFile,"%f, \n",mPSFontInfo->mAFMCharMetrics[i].mLly);
    fprintf(aOutFile,"%f, \n",mPSFontInfo->mAFMCharMetrics[i].mUrx);
    fprintf(aOutFile,"%f \n",mPSFontInfo->mAFMCharMetrics[i].mUry);
    
    fprintf(aOutFile,"}\n");
    if ( i != mPSFontInfo->mNumCharacters - 1 )
	fputc( ',', aOutFile ); 
    fputc( '\n', aOutFile );
  }
}

