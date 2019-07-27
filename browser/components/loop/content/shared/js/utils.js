





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.utils = (function() {
  "use strict";

  


  var CALL_TYPES = {
    AUDIO_VIDEO: "audio-video",
    AUDIO_ONLY: "audio"
  };

  





  function formatDate(timestamp) {
    var date = (new Date(timestamp * 1000));
    var options = {year: "numeric", month: "long", day: "numeric"};
    return date.toLocaleDateString(navigator.language, options);
  }

  



  function getTargetPlatform() {
    var platform="unknown_platform";

    if (navigator.platform.indexOf("Win") !== -1) {
      platform = "windows";
    }
    if (navigator.platform.indexOf("Mac") !== -1) {
      platform = "mac";
    }
    if (navigator.platform.indexOf("Linux") !== -1) {
      platform = "linux";
    }

    return platform;
  }

  








  function getBoolPreference(prefName) {
    if (navigator.mozLoop) {
      return !!navigator.mozLoop.getLoopBoolPref(prefName);
    }

    return !!localStorage.getItem(prefName);
  }

  


  function Helper() {
    this._iOSRegex = /^(iPad|iPhone|iPod)/;
  }

  Helper.prototype = {
    isFirefox: function(platform) {
      return platform.indexOf("Firefox") !== -1;
    },

    isFirefoxOS: function(platform) {
      
      
      
      
      
      return !!window.MozActivity && /mobi/i.test(platform);
    },

    isIOS: function(platform) {
      return this._iOSRegex.test(platform);
    },

    locationHash: function() {
      return window.location.hash;
    }
  };

  return {
    CALL_TYPES: CALL_TYPES,
    Helper: Helper,
    formatDate: formatDate,
    getTargetPlatform: getTargetPlatform,
    getBoolPreference: getBoolPreference
  };
})();
