




var loop = loop || {};















loop.standaloneMedia = (function() {
  "use strict";

  function patchSymbolIfExtant(objectName, propertyName, replacement) {
    var object;
    if (window[objectName]) {
      object = window[objectName];
    }
    if (object && object[propertyName]) {
      object[propertyName] = replacement;
    }
  }

  
  
  navigator.originalGum = navigator.getUserMedia ||
                          navigator.mozGetUserMedia ||
                          navigator.webkitGetUserMedia;

  function _MultiplexGum() {
    this.reset();
  }

  _MultiplexGum.prototype = {
    




    getPermsAndCacheMedia: function(constraints, onSuccess, onError) {
      function handleResult(callbacks, param) {
        
        
        this.userMedia.successCallbacks = [];
        this.userMedia.errorCallbacks = [];
        callbacks.forEach(function(cb) {
          if (typeof cb == "function") {
            cb(param);
          }
        })
      }
      function handleSuccess(localStream) {
        this.userMedia.pending = false;
        this.userMedia.localStream = localStream;
        this.userMedia.error = null;
        handleResult.call(this, this.userMedia.successCallbacks.slice(0), localStream);
      }

      function handleError(error) {
        this.userMedia.pending = false;
        this.userMedia.error = error;
        handleResult.call(this, this.userMedia.errorCallbacks.slice(0), error);
        this.error = null;
      }

      if (this.userMedia.localStream &&
          this.userMedia.localStream.ended) {
        this.userMedia.localStream = null;
      }

      this.userMedia.errorCallbacks.push(onError);
      this.userMedia.successCallbacks.push(onSuccess);

      if (this.userMedia.localStream) {
        handleSuccess.call(this, this.userMedia.localStream);
        return;
      } else if (this.userMedia.error) {
        handleError.call(this, this.userMedia.error);
        return;
      }

      if (this.userMedia.pending) {
        return;
      }
      this.userMedia.pending = true;

      navigator.originalGum(constraints, handleSuccess.bind(this),
        handleError.bind(this));
    },

    






    reset: function() {
      
      if (this.userMedia) {
        this.userMedia.errorCallbacks.forEach(function(cb) {
          if (typeof cb == "function") {
            cb("PERMISSION_DENIED");
          }
        });
        if (this.userMedia.localStream &&
            typeof this.userMedia.localStream.stop == "function") {
          this.userMedia.localStream.stop();
        }
      }
      this.userMedia = {
        error: null,
        localStream: null,
        pending: false,
        errorCallbacks: [],
        successCallbacks: [],
      };
    }
  };

  var singletonMultiplexGum = new _MultiplexGum();
  function myGetUserMedia() {
    
    
    singletonMultiplexGum.getPermsAndCacheMedia.apply(singletonMultiplexGum, arguments);
  };
  patchSymbolIfExtant("navigator", "mozGetUserMedia", myGetUserMedia);
  patchSymbolIfExtant("navigator", "webkitGetUserMedia", myGetUserMedia);
  patchSymbolIfExtant("navigator", "getUserMedia", myGetUserMedia);
  patchSymbolIfExtant("TBPlugin", "getUserMedia", myGetUserMedia);

  return {
    multiplexGum: singletonMultiplexGum,
    _MultiplexGum: _MultiplexGum,
    setSingleton: function(singleton) {
      singletonMultiplexGum = singleton;
    },
  };
})();
