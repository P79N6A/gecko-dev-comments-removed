





































#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN

namespace mozilla {
namespace widget {


nsresult StartAudioSession();


nsresult GetAudioSessionData(nsID& aID,
                             nsString& aSessionName,
                             nsString& aIconPath);



nsresult RecvAudioSessionData(const nsID& aID,
                              const nsString& aSessionName,
                              const nsString& aIconPath);


nsresult StopAudioSession();

} 
} 

#endif 
