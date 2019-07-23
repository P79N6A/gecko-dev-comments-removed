































































#ifndef nsMimeMapper_h__
#define nsMimeMapper_h__

#include <utility>
#include <vector>
#include <Types.h>
#include "nsString.h"

using std::pair;

class nsMimeMapperMac 
{
public:
  enum { kMappingFlavor = 'MOZm' } ;
  
  nsMimeMapperMac ( const char* inMappings = nsnull ) ;
  ~nsMimeMapperMac ( ) ;
   
    
    
  ResType MapMimeTypeToMacOSType ( const char* aMimeStr, PRBool inAddIfNotPresent = PR_TRUE ) ;
  void MapMacOSTypeToMimeType ( ResType inMacType, nsCAutoString & outMimeStr ) ;
 
    
    
    
  char* ExportMapping ( short * outLength ) const;

  static ResType MappingFlavor ( ) { return kMappingFlavor; }
  
private:

  void ParseMappings ( const char* inMappings ) ;
  
  typedef pair<ResType, nsCAutoString>	MimePair;
  typedef std::vector<MimePair>		MimeMap;
  typedef MimeMap::iterator			MimeMapIterator;
  typedef MimeMap::const_iterator	MimeMapConstIterator;
  
    
  MimeMap mMappings;
    
  short mCounter;

   
  nsMimeMapperMac ( const nsMimeMapperMac & ) ;
 
}; 


#endif
