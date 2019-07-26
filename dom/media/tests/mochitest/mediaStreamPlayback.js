



var TIMEOUT_LENGTH = 10000;










function MediaStreamPlayback(mediaElement, mediaStream) {
  this.mediaElement = mediaElement;
  this.mediaStream = mediaStream;
}

MediaStreamPlayback.prototype = {

  










  playMedia : function MSP_playMedia(isResume, onSuccess, onError) {
    var self = this;

    this.startMedia(isResume, function() {
      self.stopMediaElement();
      onSuccess();
    }, onError);
  },

  









  startMedia : function MSP_startMedia(isResume, onSuccess, onError) {
    var self = this;
    var canPlayThroughFired = false;

    
    if (!isResume) {
      is(this.mediaStream.currentTime, 0,
         "Before starting the media element, currentTime = 0");
    }

    





    var canPlayThroughCallback = function() {
      
      canPlayThroughFired = true;
      self.mediaElement.removeEventListener('canplaythrough',
        canPlayThroughCallback, false);

      is(self.mediaElement.paused, false,
        "Media element should be playing");
      is(self.mediaElement.duration, Number.POSITIVE_INFINITY,
        "Duration should be infinity");

      
      
      
      ok(self.mediaElement.readyState === HTMLMediaElement.HAVE_ENOUGH_DATA ||
         self.mediaElement.readyState === HTMLMediaElement.HAVE_CURRENT_DATA,
         "Ready state shall be HAVE_ENOUGH_DATA or HAVE_CURRENT_DATA");

      is(self.mediaElement.seekable.length, 0,
         "Seekable length shall be zero");
      is(self.mediaElement.buffered.length, 0,
         "Buffered length shall be zero");

      is(self.mediaElement.seeking, false,
         "MediaElement is not seekable with MediaStream");
      ok(isNaN(self.mediaElement.startOffsetTime),
         "Start offset time shall not be a number");
      is(self.mediaElement.loop, false, "Loop shall be false");
      is(self.mediaElement.preload, "", "Preload should not exist");
      is(self.mediaElement.src, "", "No src should be defined");
      is(self.mediaElement.currentSrc, "",
         "Current src should still be an empty string");

      var timeUpdateFired = false;

      var timeUpdateCallback = function() {
        if (self.mediaStream.currentTime > 0 &&
            self.mediaElement.currentTime > 0) {
          timeUpdateFired = true;
          self.mediaElement.removeEventListener('timeupdate',
            timeUpdateCallback, false);
          onSuccess();
        }
      };

      
      
      self.mediaElement.addEventListener('timeupdate', timeUpdateCallback,
        false);

      
      setTimeout(function() {
        if (!timeUpdateFired) {
          self.mediaElement.removeEventListener('timeupdate',
            timeUpdateCallback, false);
          ok(false, "timeUpdate event never fired");
          onError();
        }
      }, TIMEOUT_LENGTH);
    };

    
    
    this.mediaElement.addEventListener('canplaythrough', canPlayThroughCallback,
      false);

    
    this.mediaElement.mozSrcObject = this.mediaStream;
    this.mediaElement.play();

    
    setTimeout(function() {
      if (!canPlayThroughFired) {
        self.mediaElement.removeEventListener('canplaythrough',
          canPlayThroughCallback, false);
        ok(false, "canplaythrough event never fired");
        onError();
      }
    }, TIMEOUT_LENGTH);
  },

  





  stopMediaElement : function MSP_stopMediaElement() {
    this.mediaElement.pause();
    this.mediaElement.mozSrcObject = null;
  }
}










function LocalMediaStreamPlayback(mediaElement, mediaStream) {
  ok(mediaStream instanceof LocalMediaStream,
     "Stream should be a LocalMediaStream");
  MediaStreamPlayback.call(this, mediaElement, mediaStream);
}


LocalMediaStreamPlayback.prototype = new MediaStreamPlayback();
LocalMediaStreamPlayback.prototype.constructor = LocalMediaStreamPlayback;














LocalMediaStreamPlayback.prototype.playMediaWithStreamStop = function(
  isResume, onSuccess, onError) {
  var self = this;

  this.startMedia(isResume, function() {
    self.stopStreamInMediaPlayback(function() {
      self.stopMediaElement();
      onSuccess();
    }, onError);
  }, onError);
}














LocalMediaStreamPlayback.prototype.stopStreamInMediaPlayback = function(
  onSuccess, onError) {
  var endedFired = false;
  var self = this;

  



  var endedCallback = function() {
    endedFired = true;
    self.mediaElement.removeEventListener('ended', endedCallback, false);
    ok(true, "ended event successfully fired");
    onSuccess();
  };

  this.mediaElement.addEventListener('ended', endedCallback, false);
  this.mediaStream.stop();

  
  setTimeout(function() {
    if (!endedFired) {
      ok(false, "ended event never fired");
      onError();
    }
  }, TIMEOUT_LENGTH);
}
