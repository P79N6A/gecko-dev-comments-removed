(function(global) {
  'use strict';

  
  
  
  function check(constraintApplied, condition, message) {
    var good = constraintApplied ? condition : !condition;
    message = (constraintApplied ? 'with' : 'without') +
      ' constraint: should ' + (constraintApplied ? '' : 'not ') +
      message + ' = ' + (good ? 'OK' : 'waiting...');
    info(message);
    return good;
  }

  function isSilence(audioData) {
    var silence = true;
    for (var i = 0; i < audioData.length; ++i) {
      if (audioData[i] !== 128) {
        silence = false;
      }
    }
    return silence;
  }

  function periodicCheck(type, checkFunc, successMessage, done) {
    var interval = setInterval(function periodic() {
      if (checkFunc()) {
        ok(true, type + ' is ' + successMessage);
        clearInterval(interval);
        interval = null;
        done();
      }
    }, 200);
    return function cancel() {
      if (interval) {
        ok(false, 'timed out waiting for audio check');
        clearInterval(interval);
        done();
      }
    };
  }

  function checkAudio(constraintApplied, stream, done) {
    var context = new AudioContext();
    var source = context.createMediaStreamSource(stream);
    var analyser = context.createAnalyser();
    source.connect(analyser);
    analyser.connect(context.destination);

    function testAudio() {
      var sampleCount = analyser.frequencyBinCount;
      info('got some audio samples: ' + sampleCount);
      var bucket = new ArrayBuffer(sampleCount);
      var view = new Uint8Array(bucket);
      analyser.getByteTimeDomainData(view);

      var silent = check(constraintApplied, isSilence(view), 'be silence for audio');
      
      silent = constraintApplied ? !silent : silent;
      return sampleCount > 0 && silent;
    }
    return periodicCheck('audio', testAudio,
                         (constraintApplied ? '' : 'not ') + 'silent', done);
  }

  function mkElement(type) {
    var display = document.getElementById('display');
    var e = document.createElement(type);
    e.width = 32;
    e.height = 24;
    display.appendChild(e);
    return e;
  }

  function checkVideo(constraintApplied, stream, done) {
    var video = mkElement('video');
    video.mozSrcObject = stream;

    var ready = false;
    video.onplaying = function() {
      ready = true;
    }
    video.play();

    function tryToRenderToCanvas() {
      if (!ready) {
        info('waiting for video to start');
        return false;
      }

      try {
        
        
        var canvas = mkElement('canvas');
        var ctx = canvas.getContext('2d');
        
        
        
        ctx.drawImage(video, 0, 0);
        ctx.getImageData(0, 0, 1, 1);
        return check(constraintApplied, false, 'throw on getImageData for video');
      } catch (e) {
        return check(constraintApplied, e.name === 'SecurityError', 'get a security error');
      }
    }

    return periodicCheck('video', tryToRenderToCanvas,
                         (constraintApplied ? '' : 'not ') + 'protected', done);
  }

  global.audioIsSilence = checkAudio;
  global.videoIsBlack = checkVideo;
}(this));
