





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.utils = (function() {
  "use strict";

  



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

  return {
    getTargetPlatform: getTargetPlatform
  };
})();
