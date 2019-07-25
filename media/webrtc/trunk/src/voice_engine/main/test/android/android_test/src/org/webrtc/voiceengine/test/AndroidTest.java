














package org.webrtc.voiceengine.test;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;

import android.app.Activity;
import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

public class AndroidTest extends Activity {
    private byte[] _playBuffer = null;
    private short[] _circBuffer = new short[8000]; 

    private int _recIndex = 0;
    private int _playIndex = 0;
    
    private int _maxVolume = 0; 
    
    private int _volumeLevel = 204;

    private Thread _playThread;
    private Thread _recThread;
    private Thread _autotestThread;

    private static AudioTrack _at;
    private static AudioRecord _ar;

    private File _fr = null;
    private FileInputStream _in = null;

    private boolean _isRunningPlay = false;
    private boolean _isRunningRec = false;
    private boolean _settingSet = true;
    private boolean _isCallActive = false;
    private boolean _runAutotest = false; 

    private int _channel = -1;
    private int _codecIndex = 0;
    private int _ecIndex = 0;
    private int _nsIndex = 0;
    private int _agcIndex = 0;
    private int _vadIndex = 0;
    private int _audioIndex = 3;
    private int _settingMenu = 0;
    private int _receivePort = 1234;
    private int _destinationPort = 1234;
    private String _destinationIP = "127.0.0.1";

    
    private final boolean _playFromFile = false;
    
    private final boolean _runThroughNativeLayer = true;
    private final boolean enableSend = true;
    private final boolean enableReceive = true;
    private final boolean useNativeThread = false;

    
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        TextView tv = (TextView) findViewById(R.id.TextView01);
        tv.setText("");

        final EditText ed = (EditText) findViewById(R.id.EditText01);
        ed.setWidth(200);
        ed.setText(_destinationIP);

        final Button buttonStart = (Button) findViewById(R.id.Button01);
        buttonStart.setWidth(200);
        if (_runAutotest) {
            buttonStart.setText("Run test");
        } else {
            buttonStart.setText("Start Call");
        }
        
        buttonStart.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {

                if (_runAutotest) {
                    startAutoTest();
                } else {
                    if (_isCallActive) {

                        if (stopCall() != -1) {
                            _isCallActive = false;
                            buttonStart.setText("Start Call");
                        }
                    } else {

                        _destinationIP = ed.getText().toString();
                        if (startCall() != -1) {
                            _isCallActive = true;
                            buttonStart.setText("Stop Call");
                        }
                    }
                }

                
                
                
                
                
                
                
            }
        });

        final Button buttonStop = (Button) findViewById(R.id.Button02);
        buttonStop.setWidth(200);
        buttonStop.setText("Close app");
        buttonStop.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {

                if (!_runAutotest) {
                    ShutdownVoE();
                }

                
                finish();

                
                
                
                
                
                
            }
        });


        String ap1[] = {"EC off", "AECM"};
        final ArrayAdapter<String> adapterAp1 = new ArrayAdapter<String>(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        ap1);
        String ap2[] =
                        {"NS off", "NS low", "NS moderate", "NS high",
                                        "NS very high"};
        final ArrayAdapter<String> adapterAp2 = new ArrayAdapter<String>(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        ap2);
        String ap3[] = {"AGC off", "AGC adaptive", "AGC fixed"};
        final ArrayAdapter<String> adapterAp3 = new ArrayAdapter<String>(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        ap3);
        String ap4[] =
                        {"VAD off", "VAD conventional", "VAD high rate",
                                        "VAD mid rate", "VAD low rate"};
        final ArrayAdapter<String> adapterAp4 = new ArrayAdapter<String>(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        ap4);
        String codecs[] = {"iSAC", "PCMU", "PCMA", "iLBC"};
        final ArrayAdapter<String> adapterCodecs = new ArrayAdapter<String>(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        codecs);

        final Spinner spinnerSettings1 = (Spinner) findViewById(R.id.Spinner01);
        final Spinner spinnerSettings2 = (Spinner) findViewById(R.id.Spinner02);
        spinnerSettings1.setMinimumWidth(200);
        String settings[] =
                        {"Codec", "Echo Control", "Noise Suppression",
                         "Automatic Gain Control",
                         "Voice Activity Detection"};
        ArrayAdapter<String> adapterSettings1 = new ArrayAdapter<String>(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        settings);
        spinnerSettings1.setAdapter(adapterSettings1);
        spinnerSettings1.setOnItemSelectedListener(
                        new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView adapterView, View view,
                            int position, long id) {

                _settingMenu = position;
                _settingSet = false;
                if (position == 0) {
                    spinnerSettings2.setAdapter(adapterCodecs);
                    spinnerSettings2.setSelection(_codecIndex);
                }
                if (position == 1) {
                    spinnerSettings2.setAdapter(adapterAp1);
                    spinnerSettings2.setSelection(_ecIndex);
                }
                if (position == 2) {
                    spinnerSettings2.setAdapter(adapterAp2);
                    spinnerSettings2.setSelection(_nsIndex);
                }
                if (position == 3) {
                    spinnerSettings2.setAdapter(adapterAp3);
                    spinnerSettings2.setSelection(_agcIndex);
                }
                if (position == 4) {
                    spinnerSettings2.setAdapter(adapterAp4);
                    spinnerSettings2.setSelection(_vadIndex);
                }
            }

            public void onNothingSelected(AdapterView adapterView) {
                WebrtcLog("No setting1 selected");
            }
        });

        spinnerSettings2.setMinimumWidth(200);
        ArrayAdapter<String> adapterSettings2 = new ArrayAdapter<String>(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        codecs);
        spinnerSettings2.setAdapter(adapterSettings2);
        spinnerSettings2.setOnItemSelectedListener(
                        new AdapterView.OnItemSelectedListener() {
            public void onItemSelected(AdapterView adapterView, View view,
                            int position, long id) {

                
                if (_settingSet == false) {
                    _settingSet = true;
                    return;
                }

                
                if (_settingMenu == 0) {
                    WebrtcLog("Selected audio " + position);
                    setAudioProperties(position);
                    spinnerSettings2.setSelection(_audioIndex);
                }

                
                if (_settingMenu == 1) {
                    _codecIndex = position;
                    WebrtcLog("Selected codec " + position);
                    if (0 != SetSendCodec(_channel, _codecIndex)) {
                        WebrtcLog("VoE set send codec failed");
                    }
                }

                
                if (_settingMenu == 2) {
                    boolean enable = true;
                    int ECmode = 5; 
                    int AESmode = 0;

                    _ecIndex = position;
                    WebrtcLog("Selected EC " + position);

                    if (position == 0) {
                        enable = false;
                    }
                    if (position > 1) {
                        ECmode = 4; 
                        AESmode = position - 1;
                    }

                    if (0 != SetECStatus(enable, ECmode)) {
                        WebrtcLog("VoE set EC status failed");
                    }
                }

                
                if (_settingMenu == 3) {
                    boolean enable = true;

                    _nsIndex = position;
                    WebrtcLog("Selected NS " + position);

                    if (position == 0) {
                        enable = false;
                    }
                    if (0 != SetNSStatus(enable, position + 2)) {
                        WebrtcLog("VoE set NS status failed");
                    }
                }

                
                if (_settingMenu == 4) {
                    boolean enable = true;

                    _agcIndex = position;
                    WebrtcLog("Selected AGC " + position);

                    if (position == 0) {
                        enable = false;
                        position = 1; 
                    }
                    if (0 != SetAGCStatus(enable, position + 2)) {
                        WebrtcLog("VoE set AGC status failed");
                    }
                }

                
                if (_settingMenu == 5) {
                    boolean enable = true;

                    _vadIndex = position;
                    WebrtcLog("Selected VAD " + position);

                    if (position == 0) {
                        enable = false;
                        position++;
                    }
                    if (0 != SetVADStatus(_channel, enable, position - 1)) {
                        WebrtcLog("VoE set VAD status failed");
                    }
                }
            }

            public void onNothingSelected(AdapterView adapterView) {
            }
        });

        
        if (!_runAutotest && !useNativeThread) SetupVoE();

        
        
        setVolumeControlStream(AudioManager.STREAM_VOICE_CALL);

        
        
        AudioManager am =
                        (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        _maxVolume = am.getStreamMaxVolume(AudioManager.STREAM_VOICE_CALL);
        if (_maxVolume <= 0) {
            WebrtcLog("Could not get max volume!");
        } else {
            int androidVolumeLevel = (_volumeLevel * _maxVolume) / 255;
            _volumeLevel = (androidVolumeLevel * 255) / _maxVolume;
        }

        WebrtcLog("Started Webrtc Android Test");
    }

    
    
    
    protected void onDestroy() {
        super.onDestroy();
        
    }

    private void SetupVoE() {
        
        Create(); 

        
        if (0 != Init(false, false)) {
            WebrtcLog("VoE init failed");
        }

        
        _channel = CreateChannel();
        if (0 != _channel) {
            WebrtcLog("VoE create channel failed");
        }

    }

    private void ShutdownVoE() {
        
        if (0 != DeleteChannel(_channel)) {
            WebrtcLog("VoE delete channel failed");
        }

        
        if (0 != Terminate()) {
            WebrtcLog("VoE terminate failed");
        }

        
        Delete(); 
    }

    int startCall() {

        if (useNativeThread == true) {

            Create();
            return 0;
        }

        if (enableReceive == true) {
            
            if (0 != SetLocalReceiver(_channel, _receivePort)) {
                WebrtcLog("VoE set local receiver failed");
            }

            if (0 != StartListen(_channel)) {
                WebrtcLog("VoE start listen failed");
                return -1;
            }

            
            if (0 != SetLoudspeakerStatus(false)) {
                WebrtcLog("VoE set louspeaker status failed");
                return -1;
            }

            





            
            if (0 != StartPlayout(_channel)) {
                WebrtcLog("VoE start playout failed");
                return -1;
            }

            
            
            
            
            
            
        }

        if (enableSend == true) {
            if (0 != SetSendDestination(_channel, _destinationPort,
                            _destinationIP)) {
                WebrtcLog("VoE set send  destination failed");
                return -1;
            }

            if (0 != SetSendCodec(_channel, _codecIndex)) {
                WebrtcLog("VoE set send codec failed");
                return -1;
            }

            




            if (0 != StartSend(_channel)) {
                WebrtcLog("VoE start send failed");
                return -1;
            }

            
            
            
            
            
        }

        return 0;
    }

    int stopCall() {

        if (useNativeThread == true) {

            Delete();
            return 0;
        }

        if (enableSend == true) {
            
            




            
            if (0 != StopSend(_channel)) {
                WebrtcLog("VoE stop send failed");
                return -1;
            }
        }

        if (enableReceive == true) {
            
            
            
            

            
            if (0 != StopListen(_channel)) {
                WebrtcLog("VoE stop listen failed");
                return -1;
            }

            
            
            
            
            

            
            if (0 != StopPlayout(_channel)) {
                WebrtcLog("VoE stop playout failed");
                return -1;
            }

            
            if (0 != SetLoudspeakerStatus(true)) {
                WebrtcLog("VoE set louspeaker status failed");
                return -1;
            }
        }

        return 0;
    }

    int startAutoTest() {

        _autotestThread = new Thread(_autotestProc);
        _autotestThread.start();

        return 0;
    }

    private Runnable _autotestProc = new Runnable() {
        public void run() {
            
            
            
            RunAutoTest(1, 2);
        }
    };

    int setAudioProperties(int val) {

        
        

        if (val == 0) {
            
            
            
            

            int androidVolumeLevel = (_volumeLevel * _maxVolume) / 255;
            if (androidVolumeLevel < _maxVolume) {
                _volumeLevel = ((androidVolumeLevel + 1) * 255) / _maxVolume;
                if (0 != SetSpeakerVolume(_volumeLevel)) {
                    WebrtcLog("VoE set speaker volume failed");
                }
            }
        } else if (val == 1) {
            
            
            
            

            int androidVolumeLevel = (_volumeLevel * _maxVolume) / 255;
            if (androidVolumeLevel > 0) {
                _volumeLevel = ((androidVolumeLevel - 1) * 255) / _maxVolume;
                if (0 != SetSpeakerVolume(_volumeLevel)) {
                    WebrtcLog("VoE set speaker volume failed");
                }
            }
        } else if (val == 2) {
            
            if (0 != SetLoudspeakerStatus(true)) {
                WebrtcLog("VoE set loudspeaker status failed");
            }
            _audioIndex = 2;
        } else if (val == 3) {
            
            if (0 != SetLoudspeakerStatus(false)) {
                WebrtcLog("VoE set loudspeaker status failed");
            }
            _audioIndex = 3;
        }

        return 0;
    }

    int displayTextFromFile() {

        TextView tv = (TextView) findViewById(R.id.TextView01);
        FileReader fr = null;
        char[] fileBuffer = new char[64];

        try {
            fr = new FileReader("/sdcard/test.txt");
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            tv.setText("File not found!");
        }

        try {
            fr.read(fileBuffer);
        } catch (IOException e) {
            e.printStackTrace();
        }

        String readString = new String(fileBuffer);
        tv.setText(readString);
        

        return 0;
    }

    int recordAudioToFile() {
        File fr = null;
        
        byte[] recBuffer = new byte[320];

        int recBufSize =
                        AudioRecord.getMinBufferSize(16000,
                                        AudioFormat.CHANNEL_CONFIGURATION_MONO,
                                        AudioFormat.ENCODING_PCM_16BIT);
        AudioRecord rec =
                        new AudioRecord(MediaRecorder.AudioSource.MIC, 16000,
                                        AudioFormat.CHANNEL_CONFIGURATION_MONO,
                                        AudioFormat.ENCODING_PCM_16BIT,
                                        recBufSize);

        fr = new File("/sdcard/record.pcm");
        FileOutputStream out = null;
        try {
            out = new FileOutputStream(fr);
        } catch (FileNotFoundException e1) {
            e1.printStackTrace();
        }

        
        try {
            rec.startRecording();
        } catch (IllegalStateException e) {
            e.printStackTrace();
        }

        for (int i = 0; i < 550; i++) {
            
            int wrBytes = rec.read(recBuffer, 0, 320);

            try {
                out.write(recBuffer);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        
        try {
            rec.stop();
        } catch (IllegalStateException e) {
            e.printStackTrace();
        }

        return 0;
    }

    int playAudioFromFile() {

        File fr = null;
        
        
        
        final byte[] playBuffer = new byte[320];

        final int playBufSize =
                        AudioTrack.getMinBufferSize(16000,
                                        AudioFormat.CHANNEL_CONFIGURATION_MONO,
                                        AudioFormat.ENCODING_PCM_16BIT);
        
        
        final AudioTrack play =
                        new AudioTrack(AudioManager.STREAM_VOICE_CALL, 16000,
                                        AudioFormat.CHANNEL_CONFIGURATION_MONO,
                                        AudioFormat.ENCODING_PCM_16BIT,
                                        playBufSize, AudioTrack.MODE_STREAM);

        
        play.setPlaybackPositionUpdateListener(
                        new AudioTrack.OnPlaybackPositionUpdateListener() {

            int count = 0;

            public void onPeriodicNotification(AudioTrack track) {
                
                count += 320;
            }

            public void onMarkerReached(AudioTrack track) {

            }
        });

        
        

        fr = new File("/sdcard/record.pcm");
        FileInputStream in = null;
        try {
            in = new FileInputStream(fr);
        } catch (FileNotFoundException e1) {
            e1.printStackTrace();
        }

        
        
        
        
        

        
        


        
        try {
            play.play();
        } catch (IllegalStateException e) {
            e.printStackTrace();
        }

        
        

        
        for (int i = 0; i < 500; i++) {
            try {
                in.read(playBuffer);
            } catch (IOException e) {
                e.printStackTrace();
            }


            
            int wrBytes = play.write(playBuffer, 0, 320);

            Log.d("testWrite", "wrote");
        }

        
        try {
            play.stop();
        } catch (IllegalStateException e) {
            e.printStackTrace();
        }

        return 0;
    }

    int playAudioInThread() {

        if (_isRunningPlay) {
            return 0;
        }

        
        
        if (_playFromFile) {
            _playBuffer = new byte[320];
        } else {
            
            _playIndex = 0;
        }
        
        

        
        WebrtcLog("Creating AudioTrack object");
        final int minPlayBufSize =
                        AudioTrack.getMinBufferSize(16000,
                                        AudioFormat.CHANNEL_CONFIGURATION_MONO,
                                        AudioFormat.ENCODING_PCM_16BIT);
        WebrtcLog("Min play buf size = " + minPlayBufSize);
        WebrtcLog("Min volume = " + AudioTrack.getMinVolume());
        WebrtcLog("Max volume = " + AudioTrack.getMaxVolume());
        WebrtcLog("Native sample rate = "
                        + AudioTrack.getNativeOutputSampleRate(
                                        AudioManager.STREAM_VOICE_CALL));

        final int playBufSize = minPlayBufSize; 
        
        try {
            _at = new AudioTrack(
                            AudioManager.STREAM_VOICE_CALL,
                            16000,
                            AudioFormat.CHANNEL_CONFIGURATION_MONO,
                            AudioFormat.ENCODING_PCM_16BIT,
                            playBufSize, AudioTrack.MODE_STREAM);
        } catch (Exception e) {
            WebrtcLog(e.getMessage());
        }

        
        WebrtcLog("Notification marker pos = "
                        + _at.getNotificationMarkerPosition());
        WebrtcLog("Play head pos = " + _at.getPlaybackHeadPosition());
        WebrtcLog("Pos notification dt = "
                        + _at.getPositionNotificationPeriod());
        WebrtcLog("Playback rate = " + _at.getPlaybackRate());
        WebrtcLog("Sample rate = " + _at.getSampleRate());

        
        
        
        
        
        
        
        
        
        
        
        
        
        

        
        

        if (_playFromFile) {
            _fr = new File("/sdcard/singleUserDemo.pcm");
            try {
                _in = new FileInputStream(_fr);
            } catch (FileNotFoundException e1) {
                e1.printStackTrace();
            }
        }

        
        
        
        
        

        _isRunningPlay = true;

        
        _playThread = new Thread(_playProc);
        
        
        
        _playThread.start();

        return 0;
    }

    int stopPlayAudio() {
        if (!_isRunningPlay) {
            return 0;
        }

        _isRunningPlay = false;

        return 0;
    }

    private Runnable _playProc = new Runnable() {
        public void run() {

            
            android.os.Process.setThreadPriority(
                            android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);

            
            

            
            

            

            
            try {
                _at.play();
            } catch (IllegalStateException e) {
                e.printStackTrace();
            }

            
            int i = 0;
            for (; i < 3000 && _isRunningPlay; i++) {

                if (_playFromFile) {
                    try {
                        _in.read(_playBuffer);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                    int wrBytes = _at.write(_playBuffer, 0 , 320);
                } else {
                    int wrSamples =
                                    _at.write(_circBuffer, _playIndex * 160,
                                                    160);

                    
                    
                    

                    if (_playIndex == 49) {
                        _playIndex = 0;
                    } else {
                        _playIndex += 1;
                    }
                }

                
                
            }

            
            try {
                _at.stop();
            } catch (IllegalStateException e) {
                e.printStackTrace();
            }

            
            WebrtcLog("Test stopped, i = " + i + ", head = "
                            + _at.getPlaybackHeadPosition());
            int headPos = _at.getPlaybackHeadPosition();

            
            _at.flush();

            
            _at.release();
            _at = null;

            
            
            
            
            
            

            _isRunningPlay = false;

        }
    };

    int recAudioInThread() {

        if (_isRunningRec) {
            return 0;
        }

        
        

        
        _recIndex = 20;

        
        WebrtcLog("Creating AudioRecord object");
        final int minRecBufSize = AudioRecord.getMinBufferSize(16000,
                        AudioFormat.CHANNEL_CONFIGURATION_MONO,
                        AudioFormat.ENCODING_PCM_16BIT);
        WebrtcLog("Min rec buf size = " + minRecBufSize);
        
        
        
        
        

        final int recBufSize = minRecBufSize; 
        try {
            _ar = new AudioRecord(
                            MediaRecorder.AudioSource.MIC,
                            16000,
                            AudioFormat.CHANNEL_CONFIGURATION_MONO,
                            AudioFormat.ENCODING_PCM_16BIT,
                            recBufSize);
        } catch (Exception e) {
            WebrtcLog(e.getMessage());
        }

        
        WebrtcLog("Notification marker pos = "
                        + _ar.getNotificationMarkerPosition());
        
        WebrtcLog("Pos notification dt rec= "
                        + _ar.getPositionNotificationPeriod());
        
        
        WebrtcLog("Sample rate = " + _ar.getSampleRate());
        
        

        _isRunningRec = true;

        _recThread = new Thread(_recProc);

        _recThread.start();

        return 0;
    }

    int stopRecAudio() {
        if (!_isRunningRec) {
            return 0;
        }

        _isRunningRec = false;

        return 0;
    }

    private Runnable _recProc = new Runnable() {
        public void run() {

            
            android.os.Process.setThreadPriority(
                            android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);

            
            try {
                _ar.startRecording();
            } catch (IllegalStateException e) {
                e.printStackTrace();
            }

            
            
            int i = 0;
            int rdSamples = 0;
            short[] tempBuffer = new short[160]; 

            for (; i < 3000 && _isRunningRec; i++) {
                if (_runThroughNativeLayer) {
                    rdSamples = _ar.read(tempBuffer, 0, 160);
                    
                } else {
                    rdSamples = _ar.read(_circBuffer, _recIndex * 160, 160);

                    
                    
                    

                    if (_recIndex == 49) {
                        _recIndex = 0;
                    } else {
                        _recIndex += 1;
                    }
                }
            }

            
            try {
                _ar.stop();
            } catch (IllegalStateException e) {
                e.printStackTrace();
            }

            
            _ar.release();
            _ar = null;

            
            
            
            
            
            

            _isRunningRec = false;

            
            
            
            
        }
    };

    private void WebrtcLog(String msg) {
        Log.d("*Webrtc*", msg);
    }

    

    private native static boolean NativeInit();

    private native int RunAutoTest(int testType, int extendedSel);

    private native boolean Create();

    private native boolean Delete();

    private native int Init(boolean enableTrace, boolean useExtTrans);

    private native int Terminate();

    private native int CreateChannel();

    private native int DeleteChannel(int channel);

    private native int SetLocalReceiver(int channel, int port);

    private native int SetSendDestination(int channel, int port,
                    String ipaddr);

    private native int StartListen(int channel);

    private native int StartPlayout(int channel);

    private native int StartSend(int channel);

    private native int StopListen(int channel);

    private native int StopPlayout(int channel);

    private native int StopSend(int channel);

    private native int StartPlayingFileLocally(int channel, String fileName,
                    boolean loop);

    private native int StopPlayingFileLocally(int channel);

    private native int StartRecordingPlayout(int channel, String fileName,
                    boolean loop);

    private native int StopRecordingPlayout(int channel);

    private native int StartPlayingFileAsMicrophone(int channel,
                    String fileName, boolean loop);

    private native int StopPlayingFileAsMicrophone(int channel);

    private native int NumOfCodecs();

    private native int SetSendCodec(int channel, int index);

    private native int SetVADStatus(int channel, boolean enable, int mode);

    private native int SetNSStatus(boolean enable, int mode);

    private native int SetAGCStatus(boolean enable, int mode);

    private native int SetECStatus(boolean enable, int mode);

    private native int SetSpeakerVolume(int volume);

    private native int SetLoudspeakerStatus(boolean enable);

    






    static {
        Log.d("*Webrtc*", "Loading webrtc-voice-demo-jni...");
        System.loadLibrary("webrtc-voice-demo-jni");

        Log.d("*Webrtc*", "Calling native init...");
        if (!NativeInit()) {
            Log.e("*Webrtc*", "Native init failed");
            throw new RuntimeException("Native init failed");
        } else {
            Log.d("*Webrtc*", "Native init successful");
        }
    }
}
