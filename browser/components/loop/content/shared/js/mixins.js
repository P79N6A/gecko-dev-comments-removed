





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.mixins = (function() {
  "use strict";

  



  var rootObject = window;

  







  function setRootObject(obj) {
    console.log("loop.shared.mixins: rootObject set to " + obj);
    rootObject = obj;
  }

  





  var UrlHashChangeMixin = {
    componentDidMount: function() {
      rootObject.addEventListener("hashchange", this.onUrlHashChange, false);
    },

    componentWillUnmount: function() {
      rootObject.removeEventListener("hashchange", this.onUrlHashChange, false);
    }
  };

  




  var DocumentLocationMixin = {
    locationReload: function() {
      rootObject.location.reload();
    }
  };

  




  var DocumentTitleMixin = {
    setTitle: function(newTitle) {
      rootObject.document.title = newTitle;
    }
  };

  








  var WindowCloseMixin = {
    closeWindow: function() {
      rootObject.close();
    }
  };

  



  var DropdownMenuMixin = {
    get documentBody() {
      return rootObject.document.body;
    },

    getInitialState: function() {
      return {showMenu: false};
    },

    _onBodyClick: function(event) {
      var menuButton = this.refs["menu-button"] && this.refs["menu-button"].getDOMNode();
      if (this.refs.anchor) {
        menuButton = this.refs.anchor.getDOMNode();
      }
      
      
      if (event.target !== menuButton) {
        this.setState({ showMenu: false });
      }
    },

    _correctMenuPosition: function() {
      var menu = this.refs.menu && this.refs.menu.getDOMNode();
      if (!menu) {
        return;
      }

      
      var x, y;
      var menuNodeRect = menu.getBoundingClientRect();
      var x = menuNodeRect.left;
      var y = menuNodeRect.top;
      
      
      var bodyMargin = 10;
      var bodyRect = {
        height: this.documentBody.offsetHeight - bodyMargin,
        width: this.documentBody.offsetWidth - bodyMargin
      };

      
      var anchor = this.refs.anchor && this.refs.anchor.getDOMNode();
      if (anchor) {
        
        
        var anchorNodeRect = anchor.getBoundingClientRect();
        
        
        x = anchorNodeRect.left - (menuNodeRect.width / 2) + (anchorNodeRect.width / 2);
        y = anchorNodeRect.top - menuNodeRect.height - anchorNodeRect.height;
      }

      var overflowX = false;
      var overflowY = false;
      
      if (x + menuNodeRect.width > bodyRect.width) {
        
        x = bodyRect.width - ((anchor ? 0 : x) + menuNodeRect.width);
        overflowX = true;
      }
      
      if (y + menuNodeRect.height > bodyRect.height) {
        
        y = bodyRect.height - ((anchor ? 0 : y) + menuNodeRect.height);
        overflowY = true;
      }

      if (anchor || overflowX) {
        menu.style.marginLeft = x + "px";
      } else if (!menu.style.marginLeft) {
        menu.style.marginLeft = "auto";
      }

      if (anchor || overflowY) {
        menu.style.marginTop =  y + "px";
      } else if (!menu.style.marginLeft) {
        menu.style.marginTop = "auto";
      }
    },

    componentDidMount: function() {
      this.documentBody.addEventListener("click", this._onBodyClick);
      rootObject.addEventListener("blur", this.hideDropdownMenu);
    },

    componentWillUnmount: function() {
      this.documentBody.removeEventListener("click", this._onBodyClick);
      rootObject.removeEventListener("blur", this.hideDropdownMenu);
    },

    showDropdownMenu: function() {
      this.setState({showMenu: true});
      rootObject.setTimeout(this._correctMenuPosition, 0);
    },

    hideDropdownMenu: function() {
      this.setState({showMenu: false});
    },

    toggleDropdownMenu: function() {
      this[this.state.showMenu ? "hideDropdownMenu" : "showDropdownMenu"]();
    },
  };

  








  var DocumentVisibilityMixin = {
    _onDocumentVisibilityChanged: function(event) {
      var hidden = event.target.hidden;
      if (hidden && typeof this.onDocumentHidden === "function") {
        this.onDocumentHidden();
      }
      if (!hidden && typeof this.onDocumentVisible === "function") {
        this.onDocumentVisible();
      }
    },

    componentDidMount: function() {
      rootObject.document.addEventListener(
        "visibilitychange", this._onDocumentVisibilityChanged);
    },

    componentWillUnmount: function() {
      rootObject.document.removeEventListener(
        "visibilitychange", this._onDocumentVisibilityChanged);
    }
  };

  



  var MediaSetupMixin = {
    _videoDimensionsCache: {
      local: {},
      remote: {}
    },

    componentDidMount: function() {
      rootObject.addEventListener("orientationchange", this.updateVideoContainer);
      rootObject.addEventListener("resize", this.updateVideoContainer);
    },

    componentWillUnmount: function() {
      rootObject.removeEventListener("orientationchange", this.updateVideoContainer);
      rootObject.removeEventListener("resize", this.updateVideoContainer);
    },

    









    _updateDimensionsCache: function(which, newDimensions) {
      var cache = this._videoDimensionsCache[which];
      var cacheKeys = Object.keys(cache);
      var changed = false;
      Object.keys(newDimensions).forEach(function(videoType) {
        if (cacheKeys.indexOf(videoType) === -1) {
          cache[videoType] = newDimensions[videoType];
          cache[videoType].aspectRatio = this.getAspectRatio(cache[videoType]);
          changed = true;
          return;
        }
        if (cache[videoType].width !== newDimensions[videoType].width) {
          cache[videoType].width = newDimensions[videoType].width;
          changed = true;
        }
        if (cache[videoType].height !== newDimensions[videoType].height) {
          cache[videoType].height = newDimensions[videoType].height;
          changed = true;
        }
        if (changed) {
          cache[videoType].aspectRatio = this.getAspectRatio(cache[videoType]);
        }
      }, this);
      return changed;
    },

    









    updateVideoDimensions: function(localVideoDimensions, remoteVideoDimensions) {
      var localChanged = this._updateDimensionsCache("local", localVideoDimensions);
      var remoteChanged = this._updateDimensionsCache("remote", remoteVideoDimensions);
      if (localChanged || remoteChanged) {
        this.updateVideoContainer();
      }
    },

    




















    getAspectRatio: function(dimensions) {
      if (dimensions.width === dimensions.height) {
        return {width: 1, height: 1};
      }
      var denominator = Math.max(dimensions.width, dimensions.height);
      return {
        width: dimensions.width / denominator,
        height: dimensions.height / denominator
      };
    },

    























    getRemoteVideoDimensions: function(videoType) {
      var remoteVideoDimensions;

      if (videoType in this._videoDimensionsCache.remote) {
        var node = this._getElement("." + (videoType === "camera" ? "remote" : videoType));
        var width = node.offsetWidth;
        
        
        
        
        
        if (width) {
          remoteVideoDimensions = {
            width: width,
            height: node.offsetHeight
          };

          var ratio = this._videoDimensionsCache.remote[videoType].aspectRatio;
          
          var leadingAxis = Math.min(ratio.width, ratio.height) === ratio.width ?
            "width" : "height";
          var slaveAxis = leadingAxis === "height" ? "width" : "height";

          
          
          
          var leadingAxisFull = remoteVideoDimensions[slaveAxis] * ratio[leadingAxis] >
            remoteVideoDimensions[leadingAxis];

          if (leadingAxisFull) {
            
            var slaveAxisSize = remoteVideoDimensions[leadingAxis] / ratio[leadingAxis];

            remoteVideoDimensions.streamWidth = leadingAxis === "width" ?
              remoteVideoDimensions.width : slaveAxisSize;
            remoteVideoDimensions.streamHeight = leadingAxis === "height" ?
              remoteVideoDimensions.height: slaveAxisSize;
          } else {
            
            
            var leadingAxisSize = remoteVideoDimensions[slaveAxis] * ratio[leadingAxis];

            remoteVideoDimensions.streamWidth = leadingAxis === "height" ?
              remoteVideoDimensions.width : leadingAxisSize;
            remoteVideoDimensions.streamHeight = leadingAxis === "width" ?
              remoteVideoDimensions.height: leadingAxisSize;
          }
        }
      }

      
      
      if (!remoteVideoDimensions) {
        var node = this._getElement(".remote");
        var width = node.offsetWidth;
        var height = node.offsetHeight;
        remoteVideoDimensions = {
          width: width,
          height: height,
          streamWidth: width,
          streamHeight: height
        };
      }

      
      remoteVideoDimensions.offsetX = remoteVideoDimensions.width -
        remoteVideoDimensions.streamWidth;
      if (remoteVideoDimensions.offsetX > 0) {
        remoteVideoDimensions.offsetX /= 2;
      }
      remoteVideoDimensions.offsetY = remoteVideoDimensions.height -
        remoteVideoDimensions.streamHeight;
      if (remoteVideoDimensions.offsetY > 0) {
        remoteVideoDimensions.offsetY /= 2;
      }

      return remoteVideoDimensions;
    },

    







    updateVideoContainer: function() {
      if (this._bufferedUpdateVideo) {
        rootObject.clearTimeout(this._bufferedUpdateVideo);
        this._bufferedUpdateVideo = null;
      }

      this._bufferedUpdateVideo = rootObject.setTimeout(function() {
        
        
        
        try {
          this._bufferedUpdateVideo = null;
          var localStreamParent = this._getElement(".local .OT_publisher");
          var remoteStreamParent = this._getElement(".remote .OT_subscriber");
          var screenShareStreamParent = this._getElement('.screen .OT_subscriber');
          if (localStreamParent) {
            localStreamParent.style.width = "100%";
          }
          if (remoteStreamParent) {
            remoteStreamParent.style.height = "100%";
          }
          if (screenShareStreamParent) {
            screenShareStreamParent.style.height = "100%";
          }

          
          
          
          Object.keys(this._videoDimensionsCache.local).forEach(
            function (videoType) {
              var ratio = this._videoDimensionsCache.local[videoType].aspectRatio;
              if (videoType == "camera" && this.updateLocalCameraPosition) {
                this.updateLocalCameraPosition(ratio);
              }
            }, this);
          Object.keys(this._videoDimensionsCache.remote).forEach(
            function (videoType) {
              var ratio = this._videoDimensionsCache.remote[videoType].aspectRatio;
              if (videoType == "camera" && this.updateRemoteCameraPosition) {
                this.updateRemoteCameraPosition(ratio);
              }
            }, this);
        } catch (ex) {
          console.error("updateVideoContainer: _bufferedVideoUpdate exception:", ex);
        }
      }.bind(this), 0);
    },

    





    getDefaultPublisherConfig: function(options) {
      options = options || {};
      if (!("publishVideo" in options)) {
        throw new Error("missing option publishVideo");
      }

      
      
      return {
        insertMode: "append",
        fitMode: "contain",
        width: "100%",
        height: "100%",
        publishVideo: options.publishVideo,
        showControls: false,
      };
    },

    




    _getElement: function(className) {
      return this.getDOMNode().querySelector(className);
    }
  };

  



  var AudioMixin = {
    audio: null,
    _audioRequest: null,

    _isLoopDesktop: function() {
      return rootObject.navigator &&
             typeof rootObject.navigator.mozLoop === "object";
    },

    




    play: function(name, options) {
      if (this._isLoopDesktop() && rootObject.navigator.mozLoop.doNotDisturb) {
        return;
      }

      options = options || {};
      options.loop = options.loop || false;

      this._ensureAudioStopped();
      this._getAudioBlob(name, function(error, blob) {
        if (error) {
          console.error(error);
          return;
        }

        var url = URL.createObjectURL(blob);
        this.audio = new Audio(url);
        this.audio.loop = options.loop;
        this.audio.play();
      }.bind(this));
    },

    _getAudioBlob: function(name, callback) {
      if (this._isLoopDesktop()) {
        rootObject.navigator.mozLoop.getAudioBlob(name, callback);
        return;
      }

      var url = "shared/sounds/" + name + ".ogg";
      this._audioRequest = new XMLHttpRequest();
      this._audioRequest.open("GET", url, true);
      this._audioRequest.responseType = "arraybuffer";
      this._audioRequest.onload = function() {
        var request = this._audioRequest;
        var error;
        if (request.status < 200 || request.status >= 300) {
          error = new Error(request.status + " " + request.statusText);
          callback(error);
          return;
        }

        var type = request.getResponseHeader("Content-Type");
        var blob = new Blob([request.response], {type: type});
        callback(null, blob);
      }.bind(this);

      this._audioRequest.send(null);
    },

    


    _ensureAudioStopped: function() {
      if (this._audioRequest) {
        this._audioRequest.abort();
        delete this._audioRequest;
      }

      if (this.audio) {
        this.audio.pause();
        this.audio.removeAttribute("src");
        delete this.audio;
      }
    },

    


    componentWillUnmount: function() {
      this._ensureAudioStopped();
    }
  };

  



  var RoomsAudioMixin = {
    mixins: [AudioMixin],

    componentWillUpdate: function(nextProps, nextState) {
      var ROOM_STATES = loop.store.ROOM_STATES;

      function isConnectedToRoom(state) {
        return state === ROOM_STATES.HAS_PARTICIPANTS ||
               state === ROOM_STATES.SESSION_CONNECTED;
      }

      function notConnectedToRoom(state) {
        
        
        
        return state === ROOM_STATES.INIT ||
               state === ROOM_STATES.GATHER ||
               state === ROOM_STATES.READY ||
               state === ROOM_STATES.JOINED ||
               state === ROOM_STATES.ENDED;
      }

      
      if (notConnectedToRoom(this.state.roomState) &&
          isConnectedToRoom(nextState.roomState)) {
        this.play("room-joined");
      }

      
      if (this.state.roomState === ROOM_STATES.SESSION_CONNECTED &&
          nextState.roomState === ROOM_STATES.HAS_PARTICIPANTS) {
        this.play("room-joined-in");
      }

      if (this.state.roomState === ROOM_STATES.HAS_PARTICIPANTS &&
          nextState.roomState === ROOM_STATES.SESSION_CONNECTED) {
        this.play("room-left");
      }

      
      if (isConnectedToRoom(this.state.roomState) &&
          notConnectedToRoom(nextState.roomState)) {
        this.play("room-left");
      }

      
      if (nextState.roomState === ROOM_STATES.FAILED ||
          nextState.roomState === ROOM_STATES.FULL) {
        this.play("failure");
      }
    }
  };

  return {
    AudioMixin: AudioMixin,
    RoomsAudioMixin: RoomsAudioMixin,
    setRootObject: setRootObject,
    DropdownMenuMixin: DropdownMenuMixin,
    DocumentVisibilityMixin: DocumentVisibilityMixin,
    DocumentLocationMixin: DocumentLocationMixin,
    DocumentTitleMixin: DocumentTitleMixin,
    MediaSetupMixin: MediaSetupMixin,
    UrlHashChangeMixin: UrlHashChangeMixin,
    WindowCloseMixin: WindowCloseMixin
  };
})();
