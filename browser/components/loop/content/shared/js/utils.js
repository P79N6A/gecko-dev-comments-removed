





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.utils = (function(mozL10n) {
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

    



    locationData: function() {
      return {
        hash: window.location.hash,
        pathname: window.location.pathname
      };
    }
  };

  






  function composeCallUrlEmail(callUrl, recipient) {
    if (typeof navigator.mozLoop === "undefined") {
      console.warn("composeCallUrlEmail isn't available for Loop standalone.");
      return;
    }
    navigator.mozLoop.composeEmail(
      mozL10n.get("share_email_subject4", {
        clientShortname: mozL10n.get("clientShortname2")
      }),
      mozL10n.get("share_email_body4", {
        callUrl: callUrl,
        clientShortname: mozL10n.get("clientShortname2"),
        learnMoreUrl: navigator.mozLoop.getLoopCharPref("learnMoreUrl")
      }),
      recipient
    );
  }

  return {
    CALL_TYPES: CALL_TYPES,
    Helper: Helper,
    composeCallUrlEmail: composeCallUrlEmail,
    formatDate: formatDate,
    getTargetPlatform: getTargetPlatform,
    getBoolPreference: getBoolPreference
  };
})(document.mozL10n || navigator.mozL10n);
