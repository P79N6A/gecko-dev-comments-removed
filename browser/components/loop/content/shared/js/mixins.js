





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

    _onBodyClick: function() {
      this.setState({showMenu: false});
    },

    componentDidMount: function() {
      this.documentBody.addEventListener("click", this._onBodyClick);
      this.documentBody.addEventListener("blur", this.hideDropdownMenu);
    },

    componentWillUnmount: function() {
      this.documentBody.removeEventListener("click", this._onBodyClick);
      this.documentBody.removeEventListener("blur", this.hideDropdownMenu);
    },

    showDropdownMenu: function() {
      this.setState({showMenu: true});
    },

    hideDropdownMenu: function() {
      this.setState({showMenu: false});
    },

    toggleDropdownMenu: function() {
      this.setState({showMenu: !this.state.showMenu});
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
    componentDidMount: function() {
      rootObject.addEventListener('orientationchange', this.updateVideoContainer);
      rootObject.addEventListener('resize', this.updateVideoContainer);
    },

    componentWillUnmount: function() {
      rootObject.removeEventListener('orientationchange', this.updateVideoContainer);
      rootObject.removeEventListener('resize', this.updateVideoContainer);
    },

    



    updateVideoContainer: function() {
      var localStreamParent = this._getElement('.local .OT_publisher');
      var remoteStreamParent = this._getElement('.remote .OT_subscriber');
      if (localStreamParent) {
        localStreamParent.style.width = "100%";
      }
      if (remoteStreamParent) {
        remoteStreamParent.style.height = "100%";
      }
    },

    





    getDefaultPublisherConfig: function(options) {
      options = options || {};
      if (!"publishVideo" in options) {
        throw new Error("missing option publishVideo");
      }

      
      
      return {
        insertMode: "append",
        width: "100%",
        height: "100%",
        publishVideo: options.publishVideo,
        style: {
          audioLevelDisplayMode: "off",
          buttonDisplayMode: "off",
          nameDisplayMode: "off",
          videoDisabledDisplayMode: "off"
        }
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
