







































#if !defined(__formdata_H__)
#define __formdata_H__








typedef struct __struct_FormData

















{
    char** mNArray;
    char** mVArray;
    unsigned mNVCount;
    char* mStorage;
}
FormData;

FormData* FormData_Create(const char* inFormData)










;

void FormData_Destroy(FormData* inDestroy)





;

#endif 
