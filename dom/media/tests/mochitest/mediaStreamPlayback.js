



const TIMEUPDATE_TIMEOUT_LENGTH = 10000;
const ENDED_TIMEOUT_LENGTH = 30000;




const CANPLAYTHROUGH_TIMEOUT_LENGTH = 60000;










function MediaStreamPlayback(mediaElement, mediaStream) {
  this.mediaElement = mediaElement;
  this.mediaStream = mediaStream;
}

MediaStreamPlayback.prototype = {

  






  playMedia : function(isResume) {
    return this.startMedia(isResume)
      .then(() => this.stopMediaElement());
  },

  





  startMedia : function(isResume) {
    var canPlayThroughFired = false;

    
    if (!isResume) {
      is(this.mediaStream.currentTime, 0,
         "Before starting the media element, currentTime = 0");
    }

    return new Promise((resolve, reject) => {
      





      var canPlayThroughCallback = () => {
        
        canPlayThroughFired = true;
        this.mediaElement.removeEventListener('canplaythrough',
                                              canPlayThroughCallback, false);

        is(this.mediaElement.paused, false,
           "Media element should be playing");
        is(this.mediaElement.duration, Number.POSITIVE_INFINITY,
           "Duration should be infinity");

        
        
        
        ok(this.mediaElement.readyState === HTMLMediaElement.HAVE_ENOUGH_DATA ||
           this.mediaElement.readyState === HTMLMediaElement.HAVE_CURRENT_DATA,
           "Ready state shall be HAVE_ENOUGH_DATA or HAVE_CURRENT_DATA");

        is(this.mediaElement.seekable.length, 0,
           "Seekable length shall be zero");
        is(this.mediaElement.buffered.length, 0,
           "Buffered length shall be zero");

        is(this.mediaElement.seeking, false,
           "MediaElement is not seekable with MediaStream");
        ok(isNaN(this.mediaElement.startOffsetTime),
           "Start offset time shall not be a number");
        is(this.mediaElement.loop, false, "Loop shall be false");
        is(this.mediaElement.preload, "", "Preload should not exist");
        is(this.mediaElement.src, "", "No src should be defined");
        is(this.mediaElement.currentSrc, "",
           "Current src should still be an empty string");

        var timeUpdateCallback = () => {
          if (this.mediaStream.currentTime > 0 &&
              this.mediaElement.currentTime > 0) {
            this.mediaElement.removeEventListener('timeupdate',
                                                  timeUpdateCallback, false);
            resolve();
          }
        };

        
        
        this.mediaElement.addEventListener('timeupdate', timeUpdateCallback,
                                           false);

        
        setTimeout(() => {
          this.mediaElement.removeEventListener('timeupdate',
                                                timeUpdateCallback, false);
          reject(new Error("timeUpdate event never fired"));
        }, TIMEUPDATE_TIMEOUT_LENGTH);
      };

      
      
      this.mediaElement.addEventListener('canplaythrough', canPlayThroughCallback,
                                         false);

      
      this.mediaElement.mozSrcObject = this.mediaStream;
      this.mediaElement.play();

      
      setTimeout(() => {
        this.mediaElement.removeEventListener('canplaythrough',
                                              canPlayThroughCallback, false);
        reject(new Error("canplaythrough event never fired"));
      }, CANPLAYTHROUGH_TIMEOUT_LENGTH);
    });
  },

  





  stopMediaElement : function() {
    this.mediaElement.pause();
    this.mediaElement.mozSrcObject = null;
  }
}










function LocalMediaStreamPlayback(mediaElement, mediaStream) {
  ok(mediaStream instanceof LocalMediaStream,
     "Stream should be a LocalMediaStream");
  MediaStreamPlayback.call(this, mediaElement, mediaStream);
}

LocalMediaStreamPlayback.prototype = Object.create(MediaStreamPlayback.prototype, {

  






  playMediaWithStreamStop : {
    value: function(isResume) {
      return this.startMedia(isResume)
        .then(() => this.stopStreamInMediaPlayback())
        .then(() => this.stopMediaElement());
    }
  },

  







  stopStreamInMediaPlayback : {
    value: function () {
      return new Promise((resolve, reject) => {
        



        var endedCallback = () => {
          this.mediaElement.removeEventListener('ended', endedCallback, false);
          ok(true, "ended event successfully fired");
          resolve();
        };

        this.mediaElement.addEventListener('ended', endedCallback, false);
        this.mediaStream.stop();

        
        setTimeout(() => {
          reject(new Error("ended event never fired"));
        }, ENDED_TIMEOUT_LENGTH);
      });
    }
  }
});


function addLoadEvent() {}

var scriptsReady = Promise.all([
  "/tests/SimpleTest/SimpleTest.js",
  "head.js"
].map(script  => {
  var el = document.createElement("script");
  el.src = script;
  document.head.appendChild(el);
  return new Promise(r => el.onload = r);
}));

function createHTML(options) {
  return scriptsReady.then(() => realCreateHTML(options));
}

function runTest(f) {
  return scriptsReady.then(() => runTestWhenReady(f));
}
