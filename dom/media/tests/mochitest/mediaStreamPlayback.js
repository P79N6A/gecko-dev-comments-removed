












function MediaStreamPlayback(mediaElement, mediaStream) {

  
  this.mediaElement = mediaElement;

  
  this.mediaStream = mediaStream;

  









  this.startMedia = function(timeoutLength, onSuccess, onError) {
    var self = this;
    var canPlayThroughFired = false;

    
    ok(this.mediaStream instanceof LocalMediaStream,
      "Stream should be a LocalMediaStream");
    is(this.mediaStream.currentTime, 0,
      "Before starting the media element, currentTime = 0");

    





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
        if(self.mediaStream.currentTime > 0 &&
           self.mediaElement.currentTime > 0) {
          timeUpdateFired = true;
          self.mediaElement.removeEventListener('timeupdate', timeUpdateCallback,
            false);
          onSuccess();
        }
      };

      
      
      self.mediaElement.addEventListener('timeupdate', timeUpdateCallback,
        false);

      
      setTimeout(function() {
        if(!timeUpdateFired) {
          self.mediaElement.removeEventListener('timeupdate',
            timeUpdateCallback, false);
          ok(false, "timeUpdate event never fired");
          onError();
        }
      }, timeoutLength);
    };

    
    
    this.mediaElement.addEventListener('canplaythrough', canPlayThroughCallback,
      false);

    
    this.mediaElement.mozSrcObject = mediaStream;
    this.mediaElement.play();

    
    setTimeout(function() {
      if(!canPlayThroughFired) {
        self.mediaElement.removeEventListener('canplaythrough',
          canPlayThroughCallback, false);
        ok(false, "canplaythrough event never fired");
        onError();
      }
    }, timeoutLength);
  };

  





  this.stopMedia = function() {
    this.mediaElement.pause();
    this.mediaElement.mozSrcObject = null;
  };

  










  this.playMedia = function(timeoutLength, onSuccess, onError) {
    var self = this;

    this.startMedia(timeoutLength, function() {
      self.stopMedia();
      onSuccess();
    }, onError);
  };
}
