package org.webrtc.voiceengine.test;

import android.app.Activity;
import android.media.AudioManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class AudioDeviceAndroidTest extends Activity {
    private Thread _testThread;

    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        final Button buttonStart = (Button) findViewById(R.id.Button01);
        
        
        buttonStart.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                _testThread = new Thread(_testProc);
                _testThread.start();
            }
        });

        
        
        setVolumeControlStream(AudioManager.STREAM_VOICE_CALL);

        DoLog("Started WebRTC Android ADM Test");
    }

    private Runnable _testProc = new Runnable() {
        public void run() {
            
            
            RunTest(5);
        }
    };

    private void DoLog(String msg) {
        Log.d("*WebRTC ADM*", msg);
    }

    

    
    private native static boolean NativeInit();

    
    private native int RunTest(int testType);

    
    static {
        Log.d("*WebRTC ADM*", "Loading audio_device_android_test...");
        System.loadLibrary("audio_device_android_test");

        Log.d("*WebRTC ADM*", "Calling native init...");
        if (!NativeInit()) {
            Log.e("*WebRTC ADM*", "Native init failed");
            throw new RuntimeException("Native init failed");
        } else {
            Log.d("*WebRTC ADM*", "Native init successful");
        }
    }
}
