









package org.webrtc.voiceengine;

import java.nio.ByteBuffer;
import java.util.concurrent.locks.ReentrantLock;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.util.Log;

class WebRTCAudioDevice {
    private AudioTrack _audioTrack = null;
    private AudioRecord _audioRecord = null;

    private Context _context;
    private AudioManager _audioManager;

    private ByteBuffer _playBuffer;
    private ByteBuffer _recBuffer;
    private byte[] _tempBufPlay;
    private byte[] _tempBufRec;

    private final ReentrantLock _playLock = new ReentrantLock();
    private final ReentrantLock _recLock = new ReentrantLock();

    private boolean _doPlayInit = true;
    private boolean _doRecInit = true;
    private boolean _isRecording = false;
    private boolean _isPlaying = false;

    private int _bufferedRecSamples = 0;
    private int _bufferedPlaySamples = 0;
    private int _playPosition = 0;

    WebRTCAudioDevice() {
        try {
            _playBuffer = ByteBuffer.allocateDirect(2 * 480); 
                                                              
            _recBuffer = ByteBuffer.allocateDirect(2 * 480); 
                                                             
        } catch (Exception e) {
            DoLog(e.getMessage());
        }

        _tempBufPlay = new byte[2 * 480];
        _tempBufRec = new byte[2 * 480];
    }

    @SuppressWarnings("unused")
    private int InitRecording(int audioSource, int sampleRate) {
        
        int minRecBufSize =
                        AudioRecord.getMinBufferSize(sampleRate,
                                        AudioFormat.CHANNEL_CONFIGURATION_MONO,
                                        AudioFormat.ENCODING_PCM_16BIT);

        

        
        int recBufSize = minRecBufSize * 2;
        _bufferedRecSamples = (5 * sampleRate) / 200;
        

        
        if (_audioRecord != null) {
            _audioRecord.release();
            _audioRecord = null;
        }

        try {
            _audioRecord = new AudioRecord(
                            audioSource,
                            sampleRate,
                            AudioFormat.CHANNEL_CONFIGURATION_MONO,
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
        if (_isPlaying == false) {
            SetAudioMode(true);
        }

        
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
    private int InitPlayback(int sampleRate) {
        
        int minPlayBufSize = AudioTrack.getMinBufferSize(
            sampleRate,
            AudioFormat.CHANNEL_CONFIGURATION_MONO,
            AudioFormat.ENCODING_PCM_16BIT);

        

        int playBufSize = minPlayBufSize;
        if (playBufSize < 6000) {
            playBufSize *= 2;
        }
        _bufferedPlaySamples = 0;
        

        
        if (_audioTrack != null) {
            _audioTrack.release();
            _audioTrack = null;
        }

        try {
            _audioTrack = new AudioTrack(
                            AudioManager.STREAM_VOICE_CALL,
                            sampleRate,
                            AudioFormat.CHANNEL_CONFIGURATION_MONO,
                            AudioFormat.ENCODING_PCM_16BIT,
                            playBufSize, AudioTrack.MODE_STREAM);
        } catch (Exception e) {
            DoLog(e.getMessage());
            return -1;
        }

        
        if (_audioTrack.getState() != AudioTrack.STATE_INITIALIZED) {
            
            return -1;
        }

        

        if (_audioManager == null && _context != null) {
            _audioManager = (AudioManager)
                _context.getSystemService(Context.AUDIO_SERVICE);
        }

        
        if (_audioManager == null) {
            
            
            return 0;
        }
        return _audioManager.getStreamMaxVolume(AudioManager.STREAM_VOICE_CALL);
    }

    @SuppressWarnings("unused")
    private int StartPlayback() {
        if (_isRecording == false) {
            SetAudioMode(true);
        }

        
        try {
            _audioTrack.play();

        } catch (IllegalStateException e) {
            e.printStackTrace();
            return -1;
        }

        _isPlaying = true;
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

        if (_isPlaying == false) {
            SetAudioMode(false);
        }

        _isRecording = false;
        return 0;
    }

    @SuppressWarnings("unused")
    private int StopPlayback() {
        _playLock.lock();
        try {
            
            if (_audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
                
                try {
                    _audioTrack.stop();
                } catch (IllegalStateException e) {
                    e.printStackTrace();
                    return -1;
                }

                
                _audioTrack.flush();
            }

            
            _audioTrack.release();
            _audioTrack = null;

        } finally {
            
            
            _doPlayInit = true;
            _playLock.unlock();
        }

        if (_isRecording == false) {
            SetAudioMode(false);
        }

        _isPlaying = false;
        return 0;
    }

    @SuppressWarnings("unused")
    private int PlayAudio(int lengthInBytes) {

        int bufferedSamples = 0;

        _playLock.lock();
        try {
            if (_audioTrack == null) {
                return -2; 
                           
            }

            
            if (_doPlayInit == true) {
                try {
                    android.os.Process.setThreadPriority(
                        android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
                } catch (Exception e) {
                    DoLog("Set play thread priority failed: " + e.getMessage());
                }
                _doPlayInit = false;
            }

            int written = 0;
            _playBuffer.get(_tempBufPlay);
            written = _audioTrack.write(_tempBufPlay, 0, lengthInBytes);
            _playBuffer.rewind(); 

            

            
            _bufferedPlaySamples += (written >> 1);

            
            int pos = _audioTrack.getPlaybackHeadPosition();
            if (pos < _playPosition) { 
                _playPosition = 0; 
            }
            _bufferedPlaySamples -= (pos - _playPosition);
            _playPosition = pos;

            if (!_isRecording) {
                bufferedSamples = _bufferedPlaySamples;
            }

            if (written != lengthInBytes) {
                
                
                return -1;
            }

        } finally {
            
            
            _playLock.unlock();
        }

        return bufferedSamples;
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

        return (_bufferedPlaySamples);
    }

    @SuppressWarnings("unused")
    private int SetPlayoutSpeaker(boolean loudspeakerOn) {
        
        if (_audioManager == null && _context != null) {
            _audioManager = (AudioManager)
                _context.getSystemService(Context.AUDIO_SERVICE);
        }

        if (_audioManager == null) {
            DoLogErr("Could not change audio routing - no audio manager");
            return -1;
        }

        int apiLevel = Integer.parseInt(android.os.Build.VERSION.SDK);

        if ((3 == apiLevel) || (4 == apiLevel)) {
            
            if (loudspeakerOn) {
                
                _audioManager.setMode(AudioManager.MODE_NORMAL);
            } else {
                
                _audioManager.setMode(AudioManager.MODE_IN_CALL);
            }
        } else {
            
            if ((android.os.Build.BRAND.equals("Samsung") ||
                            android.os.Build.BRAND.equals("samsung")) &&
                            ((5 == apiLevel) || (6 == apiLevel) ||
                            (7 == apiLevel))) {
                
                if (loudspeakerOn) {
                    
                    _audioManager.setMode(AudioManager.MODE_IN_CALL);
                    _audioManager.setSpeakerphoneOn(loudspeakerOn);
                } else {
                    
                    _audioManager.setSpeakerphoneOn(loudspeakerOn);
                    _audioManager.setMode(AudioManager.MODE_NORMAL);
                }
            } else {
                
                _audioManager.setSpeakerphoneOn(loudspeakerOn);
            }
        }

        return 0;
    }

    @SuppressWarnings("unused")
    private int SetPlayoutVolume(int level) {

        
        if (_audioManager == null && _context != null) {
            _audioManager = (AudioManager)
                _context.getSystemService(Context.AUDIO_SERVICE);
        }

        int retVal = -1;

        if (_audioManager != null) {
            _audioManager.setStreamVolume(AudioManager.STREAM_VOICE_CALL,
                            level, 0);
            retVal = 0;
        }

        return retVal;
    }

    @SuppressWarnings("unused")
    private int GetPlayoutVolume() {

        
        if (_audioManager == null && _context != null) {
            _audioManager = (AudioManager)
                _context.getSystemService(Context.AUDIO_SERVICE);
        }

        int level = -1;

        if (_audioManager != null) {
            level = _audioManager.getStreamVolume(
                AudioManager.STREAM_VOICE_CALL);
        }

        return level;
    }

    private void SetAudioMode(boolean startCall) {
        int apiLevel = Integer.parseInt(android.os.Build.VERSION.SDK);

        if (_audioManager == null && _context != null) {
            _audioManager = (AudioManager)
                _context.getSystemService(Context.AUDIO_SERVICE);
        }

        if (_audioManager == null) {
            DoLogErr("Could not set audio mode - no audio manager");
            return;
        }

        
        
        
        if ((android.os.Build.BRAND.equals("Samsung") ||
                        android.os.Build.BRAND.equals("samsung")) &&
                (8 == apiLevel)) {
            
            
            int mode = (startCall ? 4 : AudioManager.MODE_NORMAL);
            _audioManager.setMode(mode);
            if (_audioManager.getMode() != mode) {
                DoLogErr("Could not set audio mode for Samsung device");
            }
        }
    }

    final String logTag = "WebRTC AD java";

    private void DoLog(String msg) {
        Log.d(logTag, msg);
    }

    private void DoLogErr(String msg) {
        Log.e(logTag, msg);
    }
}
