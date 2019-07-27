





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

  var ROOM_INFO_FAILURES = {
    
    NO_DATA: "no_data",
    
    WEB_CRYPTO_UNSUPPORTED: "web_crypto_unsupported",
    
    NO_CRYPTO_KEY: "no_crypto_key",
    
    DECRYPT_FAILED: "decrypt_failed"
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

  










  var getOS = _.memoize(function(platform, withVersion) {
    if (!platform) {
      if ("oscpu" in window.navigator) {
        
        platform = window.navigator.oscpu.split(";")[0].trim();
      } else {
        
        platform = window.navigator.userAgent;
      }
    }

    if (!platform) {
      return "unknown";
    }

    
    var platformPart = platform.match(/\((.*)\)/);
    if (platformPart) {
      platform = platformPart[1];
    }
    platform = platform.toLowerCase().split(";");
    if (/macintosh/.test(platform[0]) || /x11/.test(platform[0])) {
      platform = platform[1];
    } else {
      if (platform[0].indexOf("win") > -1 && platform.length > 4) {
        
        platform = platform[2];
      } else {
        platform = platform[0];
      }
    }

    if (!withVersion) {
      platform = platform.replace(/\s[0-9.]+/g, "");
    }

    return platform.trim();
  }, function(platform, withVersion) {
    
    return (platform + "") + (withVersion + "");
  });

  










  var getOSVersion = _.memoize(function(platform) {
    var os = getOS(platform, true);
    var digitsRE = /\s([0-9.]+)/;

    var version = os.match(digitsRE);
    if (!version) {
      if (os.indexOf("win") > -1) {
        if (os.indexOf("xp")) {
          return { major: 5, minor: 2 };
        } else if (os.indexOf("vista") > -1) {
          return { major: 6, minor: 0 };
        }
      }
    } else {
      version = version[1];
      
      if (os.indexOf("win") > -1) {
        switch (parseFloat(version)) {
          case 98:
            return { major: 4, minor: 1 };
          case 2000:
            return { major: 5, minor: 0 };
          case 2003:
            return { major: 5, minor: 2 };
          case 7:
          case 2008:
          case 2011:
            return { major: 6, minor: 1 };
          case 8:
            return { major: 6, minor: 2 };
          case 8.1:
          case 2012:
            return { major: 6, minor: 3 };
        }
      }

      version = version.split(".");
      return {
        major: parseInt(version[0].trim(), 10),
        minor: parseInt(version[1] ? version[1].trim() : 0, 10)
      };
    }

    return { major: Infinity, minor: 0 };
  });

  



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

  
  
  if (!Uint8Array.prototype.slice) {
    Uint8Array.prototype.slice = Uint8Array.prototype.subarray;
  }

  







  function atob(base64str) {
    var strippedEncoding = base64str.replace(/[^A-Za-z0-9\+\/]/g, "");
    var inLength = strippedEncoding.length;
    var outLength = inLength * 3 + 1 >> 2;
    var result = new Uint8Array(outLength);

    var mod3;
    var mod4;
    var uint24 = 0;
    var outIndex = 0;

    for (var inIndex = 0; inIndex < inLength; inIndex++) {
      mod4 = inIndex & 3;
      uint24 |= _b64ToUint6(strippedEncoding.charCodeAt(inIndex)) << 6 * (3 - mod4);

      if (mod4 === 3 || inLength - inIndex === 1) {
        for (mod3 = 0; mod3 < 3 && outIndex < outLength; mod3++, outIndex++) {
          result[outIndex] = uint24 >>> (16 >>> mod3 & 24) & 255;
        }
        uint24 = 0;
      }
    }

    return result;
  }

  







  function btoa(bytes) {
    var mod3 = 2;
    var result = "";
    var length = bytes.length;
    var uint24 = 0;

    for (var index = 0; index < length; index++) {
      mod3 = index % 3;
      if (index > 0 && (index * 4 / 3) % 76 === 0) {
        result += "\r\n";
      }
      uint24 |= bytes[index] << (16 >>> mod3 & 24);
      if (mod3 === 2 || length - index === 1) {
        result += String.fromCharCode(_uint6ToB64(uint24 >>> 18 & 63),
          _uint6ToB64(uint24 >>> 12 & 63),
          _uint6ToB64(uint24 >>> 6 & 63),
          _uint6ToB64(uint24 & 63));
        uint24 = 0;
      }
    }

    return result.substr(0, result.length - 2 + mod3) +
      (mod3 === 2 ? "" : mod3 === 1 ? "=" : "==");
  }

  







  function _b64ToUint6 (chr) {
    return chr > 64 && chr < 91  ? chr - 65 :
           chr > 96 && chr < 123 ? chr - 71 :
           chr > 47 && chr < 58  ? chr + 4  :
           chr === 43            ? 62       :
           chr === 47            ? 63       : 0;
  }

  







  function _uint6ToB64 (uint6) {
    return uint6 < 26   ? uint6 + 65 :
           uint6 < 52   ? uint6 + 71 :
           uint6 < 62   ? uint6 - 4  :
           uint6 === 62 ? 43         :
           uint6 === 63 ? 47         : 65;
  }

  







  function strToUint8Array(inString) {
    var inLength = inString.length;
    var arrayLength = 0;
    var chr;

    
    for (var mapIndex = 0; mapIndex < inLength; mapIndex++) {
      chr = inString.charCodeAt(mapIndex);
      arrayLength += chr < 0x80      ? 1 :
                     chr < 0x800     ? 2 :
                     chr < 0x10000   ? 3 :
                     chr < 0x200000  ? 4 :
                     chr < 0x4000000 ? 5 : 6;
    }

    var result = new Uint8Array(arrayLength);
    var index = 0;

    
    for (var chrIndex = 0; index < arrayLength; chrIndex++) {
      chr = inString.charCodeAt(chrIndex);
      if (chr < 128) {
        
        result[index++] = chr;
      } else if (chr < 0x800) {
        
        result[index++] = 192 + (chr >>> 6);
        result[index++] = 128 + (chr & 63);
      } else if (chr < 0x10000) {
        
        result[index++] = 224 + (chr >>> 12);
        result[index++] = 128 + (chr >>> 6 & 63);
        result[index++] = 128 + (chr & 63);
      } else if (chr < 0x200000) {
        
        result[index++] = 240 + (chr >>> 18);
        result[index++] = 128 + (chr >>> 12 & 63);
        result[index++] = 128 + (chr >>> 6 & 63);
        result[index++] = 128 + (chr & 63);
      } else if (chr < 0x4000000) {
        
        result[index++] = 248 + (chr >>> 24);
        result[index++] = 128 + (chr >>> 18 & 63);
        result[index++] = 128 + (chr >>> 12 & 63);
        result[index++] = 128 + (chr >>> 6 & 63);
        result[index++] = 128 + (chr & 63);
      } else { 
        
        result[index++] = 252 + (chr >>> 30);
        result[index++] = 128 + (chr >>> 24 & 63);
        result[index++] = 128 + (chr >>> 18 & 63);
        result[index++] = 128 + (chr >>> 12 & 63);
        result[index++] = 128 + (chr >>> 6 & 63);
        result[index++] = 128 + (chr & 63);
      }
    }

    return result;
  }

  







  function Uint8ArrayToStr(arrayBytes) {
    var result = "";
    var length = arrayBytes.length;
    var part;

    for (var index = 0; index < length; index++) {
      part = arrayBytes[index];
      result += String.fromCharCode(
        part > 251 && part < 254 && index + 5 < length ?
          
          
          (part - 252) * 1073741824 +
          (arrayBytes[++index] - 128 << 24) +
          (arrayBytes[++index] - 128 << 18) +
          (arrayBytes[++index] - 128 << 12) +
          (arrayBytes[++index] - 128 << 6) +
           arrayBytes[++index] - 128 :
        part > 247 && part < 252 && index + 4 < length ?
          
          (part - 248 << 24) +
          (arrayBytes[++index] - 128 << 18) +
          (arrayBytes[++index] - 128 << 12) +
          (arrayBytes[++index] - 128 << 6) +
           arrayBytes[++index] - 128 :
        part > 239 && part < 248 && index + 3 < length ?
          
          (part - 240 << 18) +
          (arrayBytes[++index] - 128 << 12) +
          (arrayBytes[++index] - 128 << 6) +
           arrayBytes[++index] - 128 :
        part > 223 && part < 240 && index + 2 < length ?
          
          (part - 224 << 12) +
          (arrayBytes[++index] - 128 << 6) +
           arrayBytes[++index] - 128 :
        part > 191 && part < 224 && index + 1 < length ?
          
          (part - 192 << 6) +
           arrayBytes[++index] - 128 :
          
          part
      );
    }

    return result;
  }

  return {
    CALL_TYPES: CALL_TYPES,
    FAILURE_DETAILS: FAILURE_DETAILS,
    REST_ERRNOS: REST_ERRNOS,
    WEBSOCKET_REASONS: WEBSOCKET_REASONS,
    STREAM_PROPERTIES: STREAM_PROPERTIES,
    SCREEN_SHARE_STATES: SCREEN_SHARE_STATES,
    ROOM_INFO_FAILURES: ROOM_INFO_FAILURES,
    composeCallUrlEmail: composeCallUrlEmail,
    formatDate: formatDate,
    getBoolPreference: getBoolPreference,
    getOS: getOS,
    getOSVersion: getOSVersion,
    isChrome: isChrome,
    isFirefox: isFirefox,
    isFirefoxOS: isFirefoxOS,
    isOpera: isOpera,
    getUnsupportedPlatform: getUnsupportedPlatform,
    locationData: locationData,
    atob: atob,
    btoa: btoa,
    strToUint8Array: strToUint8Array,
    Uint8ArrayToStr: Uint8ArrayToStr
  };
})(document.mozL10n || navigator.mozL10n);
