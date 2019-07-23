









































#ifndef __NS_INSTALLRESOURCES_H__
#define __NS_INSTALLRESOURCES_H__

#define NS_XPI_EOT "___END_OF_TABLE___"

typedef struct _nsXPIResourceTableItem
{
    char *resName;
    char *defaultString;
} nsXPIResourceTableItem;


class nsInstallResources
{
    public:
        
       static char* GetDefaultVal(const char* aResName);
};


#endif
