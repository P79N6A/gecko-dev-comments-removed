
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










function addFullscreenChangeContinuation(type, callback, inDoc) {
  var doc = inDoc || document;
  function checkCondition() {
    if (type == "enter") {
      return inFullscreenMode();
    } else if (type == "exit") {
      
      
      
      return doc.mozFullScreenElement || inNormalMode();
    } else {
      throw "'type' must be either 'enter', or 'exit'.";
    }
  }
  function invokeCallback(event) {
    
    
    requestAnimationFrame(() => setTimeout(() => callback(event), 0), 0);
  }
  function onFullscreenChange(event) {
    doc.removeEventListener("mozfullscreenchange", onFullscreenChange, false);
    if (checkCondition()) {
      invokeCallback(event);
      return;
    }
    var win = doc.defaultView;
    function onResize() {
      if (checkCondition()) {
        win.removeEventListener("resize", onResize, false);
        invokeCallback(event);
      }
    }
    win.addEventListener("resize", onResize, false);
  }
  doc.addEventListener("mozfullscreenchange", onFullscreenChange, false);
}


function addFullscreenErrorContinuation(callback, inDoc) {
  var doc = inDoc || document;
  var listener = function(event) {
    doc.removeEventListener("mozfullscreenerror", listener, false);
    setTimeout(function(){callback(event);}, 0);
  };
  doc.addEventListener("mozfullscreenerror", listener, false);
}

