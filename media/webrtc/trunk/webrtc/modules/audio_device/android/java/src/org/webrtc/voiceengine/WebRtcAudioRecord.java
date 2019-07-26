









package org.webrtc.voiceengine;

import java.nio.ByteBuffer;
import java.util.concurrent.locks.ReentrantLock;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.MediaRecorder.AudioSource;
import android.util.Log;

class WebRtcAudioRecord {
    private AudioRecord _audioRecord = null;

    private Context _context;

    private ByteBuffer _recBuffer;
    private byte[] _tempBufRec;

    private final ReentrantLock _recLock = new ReentrantLock();

    private boolean _doRecInit = true;
    private boolean _isRecording = false;

    private int _bufferedRecSamples = 0;

    WebRtcAudioRecord() {
        try {
            _recBuffer = ByteBuffer.allocateDirect(2 * 480); 
                                                             
        } catch (Exception e) {
            DoLog(e.getMessage());
        }

        _tempBufRec = new byte[2 * 480];
    }

    @SuppressWarnings("unused")
    private int InitRecording(int audioSource, int sampleRate) {
        if(android.os.Build.VERSION.SDK_INT>=11) {
            audioSource = AudioSource.VOICE_COMMUNICATION;
        } else {
            audioSource = AudioSource.DEFAULT;
        }
        
        int minRecBufSize = AudioRecord.getMinBufferSize(
            sampleRate,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_16BIT);

        

        
        int recBufSize = minRecBufSize * 2;
        
        
        _bufferedRecSamples = sampleRate / 200;
        

        
        if (_audioRecord != null) {
            _audioRecord.release();
            _audioRecord = null;
        }

        try {
            _audioRecord = new AudioRecord(
                            audioSource,
                            sampleRate,
                            AudioFormat.CHANNEL_IN_MONO,
                            AudioFormat.ENCODING_PCM_16BIT,
                            recBufSize);

        } catch (Exception e) {
            DoLog(e.getMessage());
            return -1;
        }

        
        if (_audioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
            
            return -1;
        }

        

        return _bufferedRecSamples;
    }

    @SuppressWarnings("unused")
    private int StartRecording() {
        
        try {
            _audioRecord.startRecording();

        } catch (IllegalStateException e) {
            e.printStackTrace();
            return -1;
        }

        _isRecording = true;
        return 0;
    }

    @SuppressWarnings("unused")
    private int StopRecording() {
        _recLock.lock();
        try {
            
            if (_audioRecord.getRecordingState() ==
              AudioRecord.RECORDSTATE_RECORDING) {
                
                try {
                    _audioRecord.stop();
                } catch (IllegalStateException e) {
                    e.printStackTrace();
                    return -1;
                }
            }

            
            _audioRecord.release();
            _audioRecord = null;

        } finally {
            
            
            _doRecInit = true;
            _recLock.unlock();
        }

        _isRecording = false;
        return 0;
    }

    @SuppressWarnings("unused")
    private int RecordAudio(int lengthInBytes) {
        _recLock.lock();

        try {
            if (_audioRecord == null) {
                return -2; 
                           
            }

            
            if (_doRecInit == true) {
                try {
                    android.os.Process.setThreadPriority(
                        android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
                } catch (Exception e) {
                    DoLog("Set rec thread priority failed: " + e.getMessage());
                }
                _doRecInit = false;
            }

            int readBytes = 0;
            _recBuffer.rewind(); 
            readBytes = _audioRecord.read(_tempBufRec, 0, lengthInBytes);
            
            _recBuffer.put(_tempBufRec);

            if (readBytes != lengthInBytes) {
                
                
                return -1;
            }

        } catch (Exception e) {
            DoLogErr("RecordAudio try failed: " + e.getMessage());

        } finally {
            
            
            _recLock.unlock();
        }

        return _bufferedRecSamples;
    }

    final String logTag = "WebRTC AR java";

    private void DoLog(String msg) {
        Log.d(logTag, msg);
    }

    private void DoLogErr(String msg) {
        Log.e(logTag, msg);
    }
}
