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
    var num = 0;
    var timeout;
    function periodic() {
      if (checkFunc()) {
        ok(true, type + ' is ' + successMessage);
        done();
      } else {
        setupNext();
      }
    }
    function setupNext() {
      
      
      
      timeout = setTimeout(periodic, 200 << num);
      num++;
    }

    setupNext();

    return function cancel() {
      if (timeout) {
        ok(false, type + ' (' + successMessage + ')' +
           ' failed after waiting full duration');
        clearTimeout(timeout);
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
      return sampleCount > 0 && silent;
    }
    function disconnect() {
      source.disconnect();
      analyser.disconnect();
      done();
    }
    return periodicCheck('audio', testAudio,
                         (constraintApplied ? '' : 'not ') + 'silent', disconnect);
  }

  function mkElement(type) {
    
    
    
    var e = document.createElement(type);
    e.width = 32;
    e.height = 24;
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
        return check(constraintApplied, e.name === 'SecurityError',
                     'get a security error: ' + e.name);
      }
    }

    return periodicCheck('video', tryToRenderToCanvas,
                         (constraintApplied ? '' : 'not ') + 'protected', done);
  }

  global.audioIsSilence = checkAudio;
  global.videoIsBlack = checkVideo;
}(this));
