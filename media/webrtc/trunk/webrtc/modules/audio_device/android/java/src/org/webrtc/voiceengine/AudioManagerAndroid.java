












package org.webrtc.voiceengine;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioManager;

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
    if (android.os.Build.VERSION.SDK_INT >=
        android.os.Build.VERSION_CODES.JELLY_BEAN_MR1) {
      String sampleRateString = audioManager.getProperty(
          AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
      if (sampleRateString != null) {
        mNativeOutputSampleRate = Integer.parseInt(sampleRateString);
      }
      String framesPerBuffer = audioManager.getProperty(
          AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
      if (framesPerBuffer != null) {
          mAudioLowLatencyOutputFrameSize = Integer.parseInt(framesPerBuffer);
      }
    }
    mAudioLowLatencySupported = context.getPackageManager().hasSystemFeature(
        PackageManager.FEATURE_AUDIO_LOW_LATENCY);
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