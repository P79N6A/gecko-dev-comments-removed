















#ifndef __DRM_MANAGER_CLIENT_H__
#define __DRM_MANAGER_CLIENT_H__

#include <utils/threads.h>
#include <binder/IInterface.h>
#include "drm_framework_common.h"

namespace android {

class DrmInfo;
class DrmRights;
class DrmMetadata;
class DrmInfoEvent;
class DrmInfoStatus;
class DrmInfoRequest;
class DrmSupportInfo;
class DrmConstraints;
class DrmConvertedStatus;
class DrmManagerClientImpl;






class DrmManagerClient {
public:
    DrmManagerClient();

    virtual ~DrmManagerClient();

public:
    class OnInfoListener: virtual public RefBase {

    public:
        virtual ~OnInfoListener() {}

    public:
        virtual void onInfo(const DrmInfoEvent& event) = 0;
    };





public:
    








    sp<DecryptHandle> openDecryptSession(int fd, off64_t offset, off64_t length);

    






    sp<DecryptHandle> openDecryptSession(const char* uri);

    






    status_t closeDecryptSession(sp<DecryptHandle> &decryptHandle);

    











    status_t consumeRights(sp<DecryptHandle> &decryptHandle, int action, bool reserve);

    









    status_t setPlaybackStatus(
            sp<DecryptHandle> &decryptHandle, int playbackStatus, int64_t position);

    








    status_t initializeDecryptUnit(
            sp<DecryptHandle> &decryptHandle, int decryptUnitId, const DrmBuffer* headerInfo);

    















    status_t decrypt(
            sp<DecryptHandle> &decryptHandle, int decryptUnitId,
            const DrmBuffer* encBuffer, DrmBuffer** decBuffer, DrmBuffer* IV = NULL);

    







    status_t finalizeDecryptUnit(
            sp<DecryptHandle> &decryptHandle, int decryptUnitId);

    









    ssize_t pread(sp<DecryptHandle> &decryptHandle,
            void* buffer, ssize_t numBytes, off64_t offset);

    







    bool validateAction(const String8& path, int action, const ActionDescription& description);





public:
    







    status_t setOnInfoListener(const sp<DrmManagerClient::OnInfoListener>& infoListener);

    










    DrmConstraints* getConstraints(const String8* path, const int action);

    








    DrmMetadata* getMetadata(const String8* path);

    







    bool canHandle(const String8& path, const String8& mimeType);

    






    DrmInfoStatus* processDrmInfo(const DrmInfo* drmInfo);

    







    DrmInfo* acquireDrmInfo(const DrmInfoRequest* drmInfoRequest);

    









    status_t saveRights(
        const DrmRights& drmRights, const String8& rightsPath, const String8& contentPath);

    






    String8 getOriginalMimeType(const String8& path);

    









    int getDrmObjectType(const String8& path, const String8& mimeType);

    







    int checkRightsStatus(const String8& path, int action);

    






    status_t removeRights(const String8& path);

    






    status_t removeAllRights();

    









    int openConvertSession(const String8& mimeType);

    











    DrmConvertedStatus* convertData(int convertId, const DrmBuffer* inputData);

    













    DrmConvertedStatus* closeConvertSession(int convertId);

    









    status_t getAllSupportInfo(int* length, DrmSupportInfo** drmSupportInfoArray);

private:
    int mUniqueId;
    sp<DrmManagerClientImpl> mDrmManagerClientImpl;
};

};

#endif 

