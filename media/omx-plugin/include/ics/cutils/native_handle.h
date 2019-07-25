















#ifndef NATIVE_HANDLE_H_
#define NATIVE_HANDLE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct native_handle
{
    int version;        
    int numFds;         
    int numInts;        
    int data[0];        
} native_handle_t;









int native_handle_close(const native_handle_t* h);









native_handle_t* native_handle_create(int numFds, int numInts);











int native_handle_delete(native_handle_t* h);


#ifdef __cplusplus
}
#endif

#endif
