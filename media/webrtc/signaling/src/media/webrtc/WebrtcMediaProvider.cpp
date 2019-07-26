






































#ifndef _USE_CPVE

#include "CC_Common.h"

#ifdef LINUX
#include "X11/Xlib.h"
#endif

#include "WebrtcMediaProvider.h"
#include "WebrtcAudioProvider.h"
#ifndef NO_WEBRTC_VIDEO
#include "WebrtcVideoProvider.h"
#endif
#include "WebrtcLogging.h"

using namespace std;

#if (defined (_MSC_VER) && defined (_DEBUG))





extern "C" __declspec(dllexport) void __cdecl _invalid_parameter_noinfo(void) {  }
#endif 

bool g_IncludeWebrtcLogging=false;

static const char* logTag = "WebrtcMediaProvider";

namespace CSF {



MediaProvider::~MediaProvider()
{
}


MediaProvider * MediaProvider::create( )
{
    LOG_WEBRTC_DEBUG( logTag, "MediaProvider::create");

    WebrtcMediaProvider* mediaProvider = new WebrtcMediaProvider();
    LOG_WEBRTC_DEBUG( logTag, "MediaProvider::create new instance");

    if ( mediaProvider->init() != 0)
    {
        LOG_WEBRTC_ERROR( logTag, "cannot initialize WebrtcMediaProvider");
        delete mediaProvider;
        return NULL;
    }

    return mediaProvider;
}



WebrtcMediaProvider::WebrtcMediaProvider( )
{
    pAudio = new WebrtcAudioProvider( this );
    if (pAudio->init() != 0)
    {
        LOG_WEBRTC_ERROR( logTag, "Error calling pAudio->Init in WebrtcMediaProvider");
    }

    
#ifndef NO_WEBRTC_VIDEO
    pVideo = new WebrtcVideoProvider( this );
#else
    pVideo = NULL;
#endif
}

WebrtcMediaProvider::~WebrtcMediaProvider()
{
#ifndef NO_WEBRTC_VIDEO
    delete pVideo;
#endif
    delete pAudio;
}

int WebrtcMediaProvider::init()
{
#ifndef NO_WEBRTC_VIDEO
    if (pVideo->init() != 0)
    {
        return -1;
    }
#endif

    return 0;
}

void WebrtcMediaProvider::shutdown()
{
}

void WebrtcMediaProvider::addMediaProviderObserver( MediaProviderObserver* observer )
{
    
    





}












































AudioControl* WebrtcMediaProvider::getAudioControl()
{
    return pAudio ? pAudio->getAudioControl() : NULL;
}

VideoControl* WebrtcMediaProvider::getVideoControl()
{
#ifndef NO_WEBRTC_VIDEO
    return pVideo ? pVideo->getMediaControl() : NULL;
#else
    return NULL;
#endif
}

webrtc::VoiceEngine* WebrtcMediaProvider::getWebrtcVoiceEngine () {
    return ((pAudio != NULL) ? pAudio->getVoiceEngine() : NULL);
}

AudioTermination* WebrtcMediaProvider::getAudioTermination()
{
    return pAudio ? pAudio->getAudioTermination() : NULL;
}

VideoTermination* WebrtcMediaProvider::getVideoTermination()
{
#ifndef NO_WEBRTC_VIDEO
    return pVideo ? pVideo->getMediaTermination() : NULL;
#else
    return NULL;
#endif
}

bool WebrtcMediaProvider::getKey(
    const unsigned char* masterKey, 
    int masterKeyLen, 
    const unsigned char* masterSalt, 
    int masterSaltLen,
    unsigned char* key,
    unsigned int keyLen
    )
{
    LOG_WEBRTC_DEBUG( logTag, "getKey() masterKeyLen = %d, masterSaltLen = %d", masterKeyLen, masterSaltLen);

    if(masterKey == NULL || masterSalt == NULL)
    {
        LOG_WEBRTC_ERROR( logTag, "getKey() masterKey or masterSalt is NULL");
        return false;
    }

    if((masterKeyLen != WEBRTC_MASTER_KEY_LENGTH) || (masterSaltLen != WEBRTC_MASTER_SALT_LENGTH))
    {
        LOG_WEBRTC_ERROR( logTag, "getKey() invalid masterKeyLen or masterSaltLen length");
        return false;
    }

    if((key == NULL) || (keyLen != WEBRTC_KEY_LENGTH))
    {
        LOG_WEBRTC_ERROR( logTag, "getKey() invalid key or keyLen");
        return false;
    }

    memcpy(key, masterKey, WEBRTC_MASTER_KEY_LENGTH);
    memcpy(key + WEBRTC_MASTER_KEY_LENGTH, masterSalt, WEBRTC_MASTER_SALT_LENGTH);

    return true;
}

} 

#endif
