











































#include "nsMimeMapper.h"
#include "nsITransferable.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "plstr.h"

#include <Drag.h>
#include <Scrap.h>


enum {
  kScrapFlavorTypeURL       = FOUR_CHAR_CODE('url '),
  kScrapFlavorTypeURLDesc   = FOUR_CHAR_CODE('urld')
};

nsMimeMapperMac::nsMimeMapperMac ( const char* inMappings )
  : mCounter(0)
{
  if (inMappings && strlen(inMappings) )
    ParseMappings ( inMappings );
}


nsMimeMapperMac::~nsMimeMapperMac ( )
{

}   












ResType
nsMimeMapperMac::MapMimeTypeToMacOSType ( const char* aMimeStr, PRBool inAddIfNotPresent )
{
  ResType format = 0;

  
  for ( MimeMapConstIterator it = mMappings.begin(); it != mMappings.end(); ++it ) {
    if ( it->second.Equals(aMimeStr) ) {
      format = it->first;
      break;
    }
  }

  
  
  
  
  
  
  
  if ( !format ) {
    if ( PL_strcmp(aMimeStr, kUnicodeMime) == 0 )
      format = kScrapFlavorTypeUnicode;
    else if ( PL_strcmp(aMimeStr, kTextMime) == 0 )
      format = kScrapFlavorTypeText;
    else if ( PL_strcmp(aMimeStr, kFileMime) == 0 )
      format = kDragFlavorTypeHFS;
    else if ( PL_strcmp(aMimeStr, kFilePromiseMime) == 0 )
      format = kDragFlavorTypePromiseHFS;
    else if ( PL_strcmp(aMimeStr, kNativeImageMime) == 0 )
      format = kScrapFlavorTypePicture;
    else if ( PL_strcmp(aMimeStr, kURLDataMime) == 0 )
      format = kScrapFlavorTypeURL;
    else if ( PL_strcmp(aMimeStr, kURLDescriptionMime) == 0 )
      format = kScrapFlavorTypeURLDesc;
#if NOT_YET
    else if ( PL_strcmp(aMimeStr, kPNGImageMime) == 0 )
      format = kScrapFlavorTypePicture;
    else if ( PL_strcmp(aMimeStr, kJPEGImageMime) == 0 )
      format = kScrapFlavorTypePicture;
    else if ( PL_strcmp(aMimeStr, kGIFImageMime) == 0 )
      format = kScrapFlavorTypePicture;
#endif 

    else if ( inAddIfNotPresent ) {
      
      
      format = mCounter++;
      format |= ('..MZ' << 16);     
 
      
      mMappings.push_back ( MimePair(format, nsCAutoString(aMimeStr)) );
    }
  
  }

  if ( inAddIfNotPresent )
    NS_ASSERTION ( format, "Didn't map mimeType to a macOS type for some reason" );   
  return format;
  
} 









void
nsMimeMapperMac::MapMacOSTypeToMimeType ( ResType inMacType, nsCAutoString & outMimeStr )
{
  switch ( inMacType ) {
  
    case kScrapFlavorTypeText:      outMimeStr = kTextMime;           break;
    case kScrapFlavorTypeUnicode:   outMimeStr = kUnicodeMime;        break;
    case kDragFlavorTypeHFS:        outMimeStr = kFileMime;           break;
    case kDragFlavorTypePromiseHFS: outMimeStr = kFilePromiseMime;    break;
    case kDragPromisedFlavor:       outMimeStr = kFilePromiseMime;    break;
    case kScrapFlavorTypeURL:       outMimeStr = kURLDataMime;        break;
    case kScrapFlavorTypeURLDesc:   outMimeStr = kURLDescriptionMime; break;
    
    
    
    case kScrapFlavorTypePicture: outMimeStr = kNativeImageMime; break;
    
    
    
    
    
    case 'EHTM':
      
  
    default:

      outMimeStr = "unknown";

      
      
      
      if ( inMacType & ('..MZ' << 16) ) {
        unsigned short index = inMacType & 0x0000FFFF;    
        if ( index < mMappings.size() )
          outMimeStr = mMappings[index].second;
        else
          NS_WARNING("Found a flavor starting with 'MZ..' that isn't one of ours!");
      }
        
  } 

} 













void
nsMimeMapperMac::ParseMappings ( const char* inMappings )
{
  if ( !inMappings )
    return;

  const char* currPosition = inMappings;
  while ( *currPosition ) {
    char mimeType[100];
    ResType flavor = nsnull;

    sscanf ( currPosition, "%ld %s ", &flavor, mimeType );
    mMappings.push_back( MimePair(flavor, nsCAutoString(mimeType)) );

    currPosition += 10 + 2 + strlen(mimeType);  
    
    ++mCounter;
  } 
  
} 











char*
nsMimeMapperMac::ExportMapping ( short * outLength ) const
{
  NS_ASSERTION ( outLength, "No out param provided" );
  if ( outLength )
    *outLength = 0;

#if 0



  ostringstream ostr;

  
  for ( MimeMapConstIterator it = mMappings.begin(); it != mMappings.end(); ++it ) {
    const char* mimeType = ToNewCString(it->second);
  	ostr << it->first << ' ' << mimeType << ' ';
  	delete [] mimeType;
  }
  
  return ostr.str().c_str();
#endif

  char* exportBuffer = nsnull;
  
  
  short len = 0;
  for ( MimeMapConstIterator it = mMappings.begin(); it != mMappings.end(); ++it ) {
    len += 10;  
    len += 2;   
    len += it->second.Length();  
  }  

  
  
  
  exportBuffer = static_cast<char*>(nsMemory::Alloc(len + 1));      
  if ( !exportBuffer )
    return nsnull;
  *exportBuffer = '\0';                          
  if ( len ) {
    char* posInString = exportBuffer;
    for ( MimeMapConstIterator it = mMappings.begin(); it != mMappings.end(); ++it ) {
      
      
      char* currMapping = new char[10 + 2 + it->second.Length() + 1];  
      char* mimeType = ToNewCString(it->second);
      if ( currMapping && mimeType ) {
        sprintf(currMapping, "%ld %s ", it->first, mimeType);
        strcat(posInString, currMapping);
        posInString += strlen(currMapping);     
      }
      nsMemory::Free ( mimeType );
      delete[] currMapping;
    }
      
    *posInString = '\0';                        
  } 
  
  if ( outLength )
    *outLength = len + 1;  
  return exportBuffer;
  
} 
