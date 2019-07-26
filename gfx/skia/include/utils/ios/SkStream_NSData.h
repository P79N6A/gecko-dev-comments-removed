






#ifndef SkStream_NSData_DEFINED
#define SkStream_NSData_DEFINED

#import <UIKit/UIKit.h>
#include "SkStream.h"





NSData* NSData_dataWithStream(SkStream* stream);






NSData* NSData_dataFromResource(const char name[], const char suffix[]);



class SkStream_NSData : public SkMemoryStream {
public:
            SkStream_NSData(NSData* data);
    virtual ~SkStream_NSData();

    static SkStream_NSData* CreateFromResource(const char name[],
                                               const char suffix[]);

private:
    NSData* fNSData;
};

#endif
