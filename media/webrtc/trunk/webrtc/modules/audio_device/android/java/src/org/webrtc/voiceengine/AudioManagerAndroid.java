












package org.webrtc.voiceengine;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

import org.mozilla.gecko.mozglue.WebRTCJNITarget;

@WebRTCJNITarget
class AudioManagerAndroid {
  
  
  private static final int DEFAULT_SAMPLING_RATE = 44100;
  
  
  
  private static final int DEFAULT_FRAMES_PER_BUFFER = 256;

  private int mNativeOutputSampleRate;
  private boolean mAudioLowLatencySupported;
  private int mAudioLowLatencyOutputFrameSize;


  @SuppressWarnings("unused")
  private AudioManagerAndroid(Context context) {
    AudioManager audioManager = (AudioManager)
        context.getSystemService(Context.AUDIO_SERVICE);

    mNativeOutputSampleRate = DEFAULT_SAMPLING_RATE;
    mAudioLowLatencyOutputFrameSize = DEFAULT_FRAMES_PER_BUFFER;
    mAudioLowLatencySupported = context.getPackageManager().hasSystemFeature(
      PackageManager.FEATURE_AUDIO_LOW_LATENCY);
    if (android.os.Build.VERSION.SDK_INT >=
        17 ) {
      try {
        Method getProperty = AudioManager.class.getMethod("getProperty", String.class);
        Field sampleRateField = AudioManager.class.getField("PROPERTY_OUTPUT_SAMPLE_RATE");
        Field framePerBufferField = AudioManager.class.getField("PROPERTY_OUTPUT_FRAMES_PER_BUFFER");
        String sampleRateKey = (String)sampleRateField.get(null);
        String framePerBufferKey = (String)framePerBufferField.get(null);
        String sampleRateString = (String)getProperty.invoke(audioManager, sampleRateKey);
        if (sampleRateString != null) {
          mNativeOutputSampleRate = Integer.parseInt(sampleRateString);
        }
        String framesPerBuffer = (String)getProperty.invoke(audioManager, sampleRateKey);
        if (framesPerBuffer != null) {
          mAudioLowLatencyOutputFrameSize = Integer.parseInt(framesPerBuffer);
        }
      } catch (Exception ex) {
        Log.w("WebRTC", "error getting low latency params", ex);
      }
    }
  }

    @SuppressWarnings("unused")
    private int getNativeOutputSampleRate() {
      return mNativeOutputSampleRate;
    }

    @SuppressWarnings("unused")
    private boolean isAudioLowLatencySupported() {
        return mAudioLowLatencySupported;
    }

    @SuppressWarnings("unused")
    private int getAudioLowLatencyOutputFrameSize() {
        return mAudioLowLatencyOutputFrameSize;
    }
}
