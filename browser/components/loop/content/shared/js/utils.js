





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.utils = (function(mozL10n) {
  "use strict";

  


  var CALL_TYPES = {
    AUDIO_VIDEO: "audio-video",
    AUDIO_ONLY: "audio"
  };

  var REST_ERRNOS = {
    INVALID_TOKEN: 105,
    EXPIRED: 111,
    USER_UNAVAILABLE: 122,
    ROOM_FULL: 202
  };

  var WEBSOCKET_REASONS = {
    ANSWERED_ELSEWHERE: "answered-elsewhere",
    BUSY: "busy",
    CANCEL: "cancel",
    CLOSED: "closed",
    MEDIA_FAIL: "media-fail",
    REJECT: "reject",
    TIMEOUT: "timeout"
  };

  var FAILURE_DETAILS = {
    MEDIA_DENIED: "reason-media-denied",
    UNABLE_TO_PUBLISH_MEDIA: "unable-to-publish-media",
    COULD_NOT_CONNECT: "reason-could-not-connect",
    NETWORK_DISCONNECTED: "reason-network-disconnected",
    EXPIRED_OR_INVALID: "reason-expired-or-invalid",
    UNKNOWN: "reason-unknown"
  };

  var STREAM_PROPERTIES = {
    VIDEO_DIMENSIONS: "videoDimensions",
    HAS_AUDIO: "hasAudio",
    HAS_VIDEO: "hasVideo"
  };

  var SCREEN_SHARE_STATES = {
    INACTIVE: "ss-inactive",
    
    PENDING: "ss-pending",
    ACTIVE: "ss-active"
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

  function isChrome(platform) {
    return platform.toLowerCase().indexOf('chrome') > -1 ||
           platform.toLowerCase().indexOf('chromium') > -1;
  }

  function isFirefox(platform) {
    return platform.toLowerCase().indexOf("firefox") !== -1;
  }

  function isFirefoxOS(platform) {
    
    
    
    
    
    return !!window.MozActivity && /mobi/i.test(platform);
  }

  function isOpera(platform) {
    return platform.toLowerCase().indexOf('opera') > -1 ||
           platform.toLowerCase().indexOf('opr') > -1;
  }

  





  function getUnsupportedPlatform(platform) {
    if (/^(iPad|iPhone|iPod)/.test(platform)) {
      return "ios";
    }

    if (/Windows Phone/i.test(platform)) {
      return "windows_phone";
    }

    if (/BlackBerry/i.test(platform)) {
      return "blackberry";
    }

    return null;
  }

  



  function locationData() {
    return {
      hash: window.location.hash,
      pathname: window.location.pathname
    };
  }

  






  function composeCallUrlEmail(callUrl, recipient) {
    if (typeof navigator.mozLoop === "undefined") {
      console.warn("composeCallUrlEmail isn't available for Loop standalone.");
      return;
    }
    navigator.mozLoop.composeEmail(
      mozL10n.get("share_email_subject5", {
        clientShortname2: mozL10n.get("clientShortname2")
      }),
      mozL10n.get("share_email_body5", {
        callUrl: callUrl,
        brandShortname: mozL10n.get("brandShortname"),
        clientShortname2: mozL10n.get("clientShortname2"),
        clientSuperShortname: mozL10n.get("clientSuperShortname"),
        learnMoreUrl: navigator.mozLoop.getLoopPref("learnMoreUrl")
      }).replace(/\r\n/g, "\n").replace(/\n/g, "\r\n"),
      recipient
    );
  }

  return {
    CALL_TYPES: CALL_TYPES,
    FAILURE_DETAILS: FAILURE_DETAILS,
    REST_ERRNOS: REST_ERRNOS,
    WEBSOCKET_REASONS: WEBSOCKET_REASONS,
    STREAM_PROPERTIES: STREAM_PROPERTIES,
    SCREEN_SHARE_STATES: SCREEN_SHARE_STATES,
    composeCallUrlEmail: composeCallUrlEmail,
    formatDate: formatDate,
    getBoolPreference: getBoolPreference,
    isChrome: isChrome,
    isFirefox: isFirefox,
    isFirefoxOS: isFirefoxOS,
    isOpera: isOpera,
    getUnsupportedPlatform: getUnsupportedPlatform,
    locationData: locationData
  };
})(document.mozL10n || navigator.mozL10n);
