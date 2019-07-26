




















'use strict';

(function() {
  
  if (MouseEventShim)
    return;

  
  
  try {
    document.createEvent('TouchEvent');
  } catch (e) {
    return;
  }

  var starttouch; 
  var target; 
  var emitclick; 

  
  window.addEventListener('mousedown', discardEvent, true);
  window.addEventListener('mouseup', discardEvent, true);
  window.addEventListener('mousemove', discardEvent, true);
  window.addEventListener('click', discardEvent, true);

  function discardEvent(e) {
    if (e.isTrusted) {
      e.stopImmediatePropagation(); 
      if (e.type === 'click')
        e.preventDefault(); 
    }
  }

  
  
  
  
  window.addEventListener('touchstart', handleTouchStart);
  window.addEventListener('touchmove', handleTouchMove);
  window.addEventListener('touchend', handleTouchEnd);
  window.addEventListener('touchcancel', handleTouchEnd); 

  function handleTouchStart(e) {
    
    if (starttouch)
      return;

    
    if (e.defaultPrevented)
      return;

    
    
    
    try {
      e.changedTouches[0].target.ownerDocument;
    }
    catch (e) {
      
      return;
    }

    
    starttouch = e.changedTouches[0];
    target = starttouch.target;
    emitclick = true;

    
    emitEvent('mousemove', target, starttouch);

    
    var result = emitEvent('mousedown', target, starttouch);

    
    
    if (!result) {
      e.preventDefault();
      emitclick = false;
    }
  }

  function handleTouchEnd(e) {
    if (!starttouch)
      return;

    
    if (MouseEventShim.capturing) {
      MouseEventShim.capturing = false;
      MouseEventShim.captureTarget = null;
    }

    for (var i = 0; i < e.changedTouches.length; i++) {
      var touch = e.changedTouches[i];
      
      if (touch.identifier !== starttouch.identifier)
        continue;

      emitEvent('mouseup', target, touch);

      
      
      
      if (emitclick)
        emitEvent('click', starttouch.target, touch);

      starttouch = null;
      return;
    }
  }

  function handleTouchMove(e) {
    if (!starttouch)
      return;

    for (var i = 0; i < e.changedTouches.length; i++) {
      var touch = e.changedTouches[i];
      
      if (touch.identifier !== starttouch.identifier)
        continue;

      
      if (e.defaultPrevented)
        return;

      
      var dx = Math.abs(touch.screenX - starttouch.screenX);
      var dy = Math.abs(touch.screenY - starttouch.screenY);
      if (dx > MouseEventShim.dragThresholdX ||
          dy > MouseEventShim.dragThresholdY) {
        emitclick = false;
      }

      var tracking = MouseEventShim.trackMouseMoves &&
        !MouseEventShim.capturing;

      if (tracking) {
        
        
        
        
        
        var oldtarget = target;
        var newtarget = document.elementFromPoint(touch.clientX, touch.clientY);
        if (newtarget === null) {
          
          newtarget = oldtarget;
        }
        if (newtarget !== oldtarget) {
          leave(oldtarget, newtarget, touch); 
          target = newtarget;
        }
      }
      else if (MouseEventShim.captureTarget) {
        target = MouseEventShim.captureTarget;
      }

      emitEvent('mousemove', target, touch);

      if (tracking && newtarget !== oldtarget) {
        enter(newtarget, oldtarget, touch); 
      }
    }
  }

  
  function contains(a, b) {
    return (a.compareDocumentPosition(b) & 16) !== 0;
  }

  
  
  function leave(oldtarget, newtarget, touch) {
    emitEvent('mouseout', oldtarget, touch, newtarget);

    
    
    
    for (var e = oldtarget; !contains(e, newtarget); e = e.parentNode) {
      emitEvent('mouseleave', e, touch, newtarget);
    }
  }

  
  
  function enter(newtarget, oldtarget, touch) {
    emitEvent('mouseover', newtarget, touch, oldtarget);

    
    
    for (var e = newtarget; !contains(e, oldtarget); e = e.parentNode) {
      emitEvent('mouseenter', e, touch, oldtarget);
    }
  }

  function emitEvent(type, target, touch, relatedTarget) {
    var synthetic = document.createEvent('MouseEvents');
    var bubbles = (type !== 'mouseenter' && type !== 'mouseleave');
    var count =
      (type === 'mousedown' || type === 'mouseup' || type === 'click') ? 1 : 0;

    synthetic.initMouseEvent(type,
                             bubbles, 
                             true, 
                             window,
                             count, 
                             touch.screenX,
                             touch.screenY,
                             touch.clientX,
                             touch.clientY,
                             false, 
                             false, 
                             false, 
                             false, 
                             0, 
                             relatedTarget || null);

    try {
      return target.dispatchEvent(synthetic);
    }
    catch (e) {
      console.warn('Exception calling dispatchEvent', type, e);
      return true;
    }
  }
}());

var MouseEventShim = {
  
  
  
  
  getEventTimestamp: function(e) {
    if (e.isTrusted) 
      return e.timeStamp;
    else
      return e.timeStamp / 1000;
  },

  
  
  trackMouseMoves: true,

  
  
  
  
  
  setCapture: function(target) {
    this.capturing = true; 
    if (target)
      this.captureTarget = target;
  },

  capturing: false,

  
  
  
  dragThresholdX: 25,
  dragThresholdY: 25
};
