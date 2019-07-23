



#ifndef CHROME_COMMON_CHROME_PATHS_INTERNAL_H_
#define CHROME_COMMON_CHROME_PATHS_INTERNAL_H_

class FilePath;

namespace chrome {



bool GetDefaultUserDataDirectory(FilePath* result);


bool GetUserDocumentsDirectory(FilePath* result);


bool GetUserDownloadsDirectory(FilePath* result);


bool GetUserDesktop(FilePath* result);

}  


#endif  
