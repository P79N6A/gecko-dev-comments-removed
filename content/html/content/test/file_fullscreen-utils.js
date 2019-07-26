
var normalSize = new function() {
  this.w = window.outerWidth;
  this.h = window.outerHeight;
}();




function inFullscreenMode() {
  return window.outerWidth == window.screen.width &&
         window.outerHeight == window.screen.height;
}




function inNormalMode() {
  return window.outerWidth == normalSize.w &&
         window.outerHeight == normalSize.h;
}

function ok(condition, msg) {
  opener.ok(condition, "[rollback] " + msg);
  if (!condition) {
    opener.finish();
  }
}




const workAroundFullscreenTransition = navigator.userAgent.indexOf("Linux") != -1;










function addFullscreenChangeContinuation(type, callback, inDoc) {
  var doc = inDoc || document;
  var listener = null;
  if (type === "enter") {
    
    
    listener = function(event) {
      doc.removeEventListener("mozfullscreenchange", listener, false);
      if (!workAroundFullscreenTransition) {
        callback(event);
        return;
      }
      if (!inFullscreenMode()) {
        opener.todo(false, "fullscreenchange before entering fullscreen complete! " +
                    " window.fullScreen=" + window.fullScreen +
                    " normal=(" + normalSize.w + "," + normalSize.h + ")" +
                    " outerWidth=" + window.outerWidth + " width=" + window.screen.width +
                    " outerHeight=" + window.outerHeight + " height=" + window.screen.height);
        setTimeout(function(){listener(event);}, 100);
        return;
      }
      setTimeout(function(){callback(event)}, 0);
    };
  } else if (type === "exit") {
    listener = function(event) {
      doc.removeEventListener("mozfullscreenchange", listener, false);
      if (!workAroundFullscreenTransition) {
        callback(event);
        return;
      }
      if (!document.mozFullScreenElement && !inNormalMode()) {
        opener.todo(false, "fullscreenchange before exiting fullscreen complete! " +
                    " window.fullScreen=" + window.fullScreen +
                    " normal=(" + normalSize.w + "," + normalSize.h + ")" +
                    " outerWidth=" + window.outerWidth + " width=" + window.screen.width +
                    " outerHeight=" + window.outerHeight + " height=" + window.screen.height);
        
        
        
        setTimeout(function(){listener(event);}, 100);
        return;
      }
      opener.info("[rollback] Exited fullscreen");
      setTimeout(function(){callback(event);}, 0);
    };
  } else {
    throw "'type' must be either 'enter', or 'exit'.";
  }
  doc.addEventListener("mozfullscreenchange", listener, false);
}


function addFullscreenErrorContinuation(callback, inDoc) {
  var doc = inDoc || document;
  var listener = function(event) {
    doc.removeEventListener("mozfullscreenerror", listener, false);
    setTimeout(function(){callback(event);}, 0);
  };
  doc.addEventListener("mozfullscreenerror", listener, false);
}

