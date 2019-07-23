



#ifndef CHROME_COMMON_QUARANTINE_MAC_H_
#define CHROME_COMMON_QUARANTINE_MAC_H_

class FilePath;
class GURL;

namespace quarantine_mac {





void AddQuarantineMetadataToFile(const FilePath& file, const GURL& source,
                                 const GURL& referrer);

}  

#endif  
