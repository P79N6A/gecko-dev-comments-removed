





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.mixins = (function() {
  "use strict";

  



  var rootObject = window;

  





  function setRootObject(obj) {
    console.info("loop.shared.mixins: rootObject set to " + obj);
    rootObject = obj;
  }

  





  var UrlHashChangeMixin = {
    propTypes: {
      onUrlHashChange: React.PropTypes.func.isRequired
    },

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

  



  var AudioMixin = {
    audio: null,

    _isLoopDesktop: function() {
      return typeof rootObject.navigator.mozLoop === "object";
    },

    




    play: function(filename, options) {
      if (this._isLoopDesktop()) {
        
        return;
      }

      options = options || {};
      options.loop = options.loop || false;

      this._ensureAudioStopped();
      this.audio = new Audio('shared/sounds/' + filename + ".ogg");
      this.audio.loop = options.loop;
      this.audio.play();
    },

    


    _ensureAudioStopped: function() {
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

  return {
    AudioMixin: AudioMixin,
    setRootObject: setRootObject,
    DropdownMenuMixin: DropdownMenuMixin,
    DocumentVisibilityMixin: DocumentVisibilityMixin,
    DocumentLocationMixin: DocumentLocationMixin,
    UrlHashChangeMixin: UrlHashChangeMixin
  };
})();
