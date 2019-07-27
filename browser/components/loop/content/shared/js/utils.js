





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.utils = (function(mozL10n) {
  "use strict";

  


  var CALL_TYPES = {
    AUDIO_VIDEO: "audio-video",
    AUDIO_ONLY: "audio"
  };

  var FAILURE_REASONS = {
    MEDIA_DENIED: "reason-media-denied",
    COULD_NOT_CONNECT: "reason-could-not-connect",
    NETWORK_DISCONNECTED: "reason-network-disconnected",
    EXPIRED_OR_INVALID: "reason-expired-or-invalid",
    UNKNOWN: "reason-unknown"
  };

  





  function formatDate(timestamp) {
    var date = (new Date(timestamp * 1000));
    var options = {year: "numeric", month: "long", day: "numeric"};
    return date.toLocaleDateString(navigator.language, options);
  }

  








  function getBoolPreference(prefName) {
    if (navigator.mozLoop) {
      return !!navigator.mozLoop.getLoopPref(prefName);
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
        learnMoreUrl: navigator.mozLoop.getLoopPref("learnMoreUrl")
      }),
      recipient
    );
  }

  return {
    CALL_TYPES: CALL_TYPES,
    FAILURE_REASONS: FAILURE_REASONS,
    Helper: Helper,
    composeCallUrlEmail: composeCallUrlEmail,
    formatDate: formatDate,
    getBoolPreference: getBoolPreference
  };
})(document.mozL10n || navigator.mozL10n);
