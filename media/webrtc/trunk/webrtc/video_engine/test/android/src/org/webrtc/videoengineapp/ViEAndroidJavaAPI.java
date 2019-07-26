









package org.webrtc.videoengineapp;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class ViEAndroidJavaAPI {

    public ViEAndroidJavaAPI(Context context) {
        Log.d("*WEBRTCJ*", "Loading ViEAndroidJavaAPI...");
        System.loadLibrary("webrtc-video-demo-jni");

        Log.d("*WEBRTCJ*", "Calling native init...");
        if (!NativeInit(context)) {
            Log.e("*WEBRTCJ*", "Native init failed");
            throw new RuntimeException("Native init failed");
        }
        else {
            Log.d("*WEBRTCJ*", "Native init successful");
        }
        String a = "";
        a.getBytes();
    }

    
    private native boolean NativeInit(Context context);

    
    
    public native int GetVideoEngine();
    public native int Init(boolean enableTrace);
    public native int Terminate();

    public native int StartSend(int channel);
    public native int StopRender(int channel);
    public native int StopSend(int channel);
    public native int StartReceive(int channel);
    public native int StopReceive(int channel);
    
    public native int CreateChannel(int voiceChannel);
    
    public native int SetLocalReceiver(int channel, int port);
    public native int SetSendDestination(int channel, int port, String ipaddr);
    
    public native String[] GetCodecs();
    public native int SetReceiveCodec(int channel, int codecNum,
            int intbitRate, int width,
            int height, int frameRate);
    public native int SetSendCodec(int channel, int codecNum,
            int intbitRate, int width,
            int height, int frameRate);
    
    public native int AddRemoteRenderer(int channel, Object glSurface);
    public native int RemoveRemoteRenderer(int channel);
    public native int StartRender(int channel);

    
    public native int StartCamera(int channel, int cameraNum);
    public native int StopCamera(int cameraId);
    public native int GetCameraOrientation(int cameraNum);
    public native int SetRotation(int cameraId,int degrees);

    
    public native int SetExternalMediaCodecDecoderRenderer(
            int channel, Object glSurface);

    
    public native int EnableNACK(int channel, boolean enable);

    
    public native int EnablePLI(int channel, boolean enable);

    
    public native int SetCallback(int channel, IViEAndroidCallback callback);

    public native int StartIncomingRTPDump(int channel, String file);
    public native int StopIncomingRTPDump(int channel);

    
    
    public native boolean VoE_Create(Context context);
    public native boolean VoE_Delete();

    
    public native int VoE_Init(boolean enableTrace);
    public native int VoE_Terminate();

    
    public native int VoE_CreateChannel();
    public native int VoE_DeleteChannel(int channel);

    
    public native int VoE_SetLocalReceiver(int channel, int port);
    public native int VoE_SetSendDestination(int channel, int port,
                                             String ipaddr);

    
    public native int VoE_StartListen(int channel);
    public native int VoE_StartPlayout(int channel);
    public native int VoE_StartSend(int channel);
    public native int VoE_StopListen(int channel);
    public native int VoE_StopPlayout(int channel);
    public native int VoE_StopSend(int channel);

    
    public native int VoE_SetSpeakerVolume(int volume);

    
    public native int VoE_SetLoudspeakerStatus(boolean enable);

    
    public native int VoE_StartPlayingFileLocally(
        int channel,
        String fileName,
        boolean loop);
    public native int VoE_StopPlayingFileLocally(int channel);

    
    public native int VoE_StartPlayingFileAsMicrophone(
        int channel,
        String fileName,
        boolean loop);
    public native int VoE_StopPlayingFileAsMicrophone(int channel);

    
    public native int VoE_NumOfCodecs();
    public native String[] VoE_GetCodecs();
    public native int VoE_SetSendCodec(int channel, int index);

    
    public native int VoE_SetECStatus(boolean enable);
    public native int VoE_SetAGCStatus(boolean enable);
    public native int VoE_SetNSStatus(boolean enable);
    public native int VoE_StartDebugRecording(String file);
    public native int VoE_StopDebugRecording();
    public native int VoE_StartIncomingRTPDump(int channel, String file);
    public native int VoE_StopIncomingRTPDump(int channel);
}
