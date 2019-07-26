
























































var SyntheticGestures = (function() {
  
  var EVENT_INTERVAL = 30;  

  
  var touches = [];
  
  var nextTouchId = 1000;

  
  
  function emitTouchEvent(type, touch) {
    var target = touch.target;
    var doc = target.ownerDocument;
    var win = doc.defaultView;

    

    
    var documentTouches = doc.createTouchList(touches.filter(function(t) {
      return t.target.ownerDocument === doc;
    }));
    
    var targetTouches = doc.createTouchList(touches.filter(function(t) {
      return t.target === target;
    }));
    
    var changedTouches = doc.createTouchList(touch);

    
    var event = document.createEvent('TouchEvent');
    event.initTouchEvent(type,
                         true, 
                         true, 
                         win,
                         0,    
                         false, false, false, false, 
                         documentTouches,
                         targetTouches,
                         changedTouches);

    
    target.dispatchEvent(event);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  function touch(target, duration, xt, yt, then) {
    var doc = target.ownerDocument;
    var win = doc.defaultView;
    var touchId = nextTouchId++;

    var x = xt;
    if (typeof xt !== 'function') {
      x = function(t) { return xt[0] + t / duration * (xt[1] - xt[0]); };
    }

    var y = yt;
    if (typeof yt !== 'function') {
      y = function(t) { return yt[0] + t / duration * (yt[1] - yt[0]); };
    }

    
    var clientX = Math.round(x(0)), clientY = Math.round(y(0));

    
    var pageX = clientX + win.pageXOffset,
        pageY = clientY + win.pageYOffset;

    
    var screenX = clientX + win.mozInnerScreenX,
        screenY = clientY + win.mozInnerScreenY;

    
    var lastX = clientX, lastY = clientY;

    
    var touch = doc.createTouch(win, target, touchId,
                                pageX, pageY,
                                screenX, screenY,
                                clientX, clientY);

    
    touches.push(touch);

    
    emitTouchEvent('touchstart', touch);

    var startTime = Date.now();

    setTimeout(nextEvent, EVENT_INTERVAL);

    function nextEvent() {
      
      var time = Date.now();
      var dt = time - startTime;
      var last = dt + EVENT_INTERVAL / 2 > duration;

      
      
      var touchIndex = touches.indexOf(touch);

      
      if (last)
        dt = duration;

      
      clientX = Math.round(x(dt));
      clientY = Math.round(y(dt));

      
      if (clientX !== lastX || clientY !== lastY) { 
        lastX = clientX;
        lastY = clientY;
        pageX = clientX + win.pageXOffset;
        pageY = clientY + win.pageYOffset;
        screenX = clientX + win.mozInnerScreenX;
        screenY = clientY + win.mozInnerScreenY;

        
        
        touch = doc.createTouch(win, target, touchId,
                                pageX, pageY,
                                screenX, screenY,
                                clientX, clientY);

        
        touches[touchIndex] = touch;

        
        emitTouchEvent('touchmove', touch);
      }

      
      
      if (last) {
        touches.splice(touchIndex, 1);
        emitTouchEvent('touchend', touch);
        if (then)
          setTimeout(then, 0);
      }
      
      else {
        setTimeout(nextEvent, EVENT_INTERVAL);
      }
    }
  }

  function coordinates(target, x0, y0, x1, y1) {
    var coords = {};
    var box = target.getBoundingClientRect();

    var tx0 = typeof x0;
    var ty0 = typeof y0;
    var tx1 = typeof x1;
    var ty1 = typeof y1;

    function percent(s, x) {
      s = s.trim();
      var f = parseFloat(s);
      if (s[s.length - 1] === '%')
        f = f * x / 100;
      return f;
    }

    function relative(s, x) {
      var factor;
      if (s[0] === '+')
        factor = 1;
      else
        factor = -1;
      return factor * percent(s.substring(1), x);
    }

    if (tx0 === 'number')
      coords.x0 = box.left + x0;
    else if (tx0 === 'string')
      coords.x0 = box.left + percent(x0, box.width);

    if (tx1 === 'number')
      coords.x1 = box.left + x1;
    else if (tx1 === 'string') {
      x1 = x1.trim();
      if (x1[0] === '+' || x1[0] === '-')
        coords.x1 = coords.x0 + relative(x1, box.width);
      else
        coords.x1 = box.left + percent(x1, box.width);
    }

    if (ty0 === 'number')
      coords.y0 = box.top + y0;
    else if (ty0 === 'string')
      coords.y0 = box.top + percent(y0, box.height);

    if (ty1 === 'number')
      coords.y1 = box.top + y1;
    else if (ty1 === 'string') {
      y1 = y1.trim();
      if (y1[0] === '+' || y1[0] === '-')
        coords.y1 = coords.y0 + relative(y1, box.height);
      else
        coords.y1 = box.top + percent(y1, box.height);
    }

    return coords;
  }



  
  
  
  
  
  function tap(target, then, x, y, t, sendAll) {
    if (!SyntheticGestures.touchSupported || !target.ownerDocument.createTouch) {
      console.warn('tap: touch events not supported; using mouse instead');
      return mousetap(target, then, x, y, t, true);
    }

    if (x == null)
      x = '50%';
    if (y == null)
      y = '50%';

    var c = coordinates(target, x, y);

    if (sendAll) {
      touch(target, t || 50, [c.x0, c.x0], [c.y0, c.y0],  function() {
        mousetap(target, then, x, y, t, true);
      });
    }
    else {
      touch(target, t || 50, [c.x0, c.x0], [c.y0, c.y0], then);
    }
  }

  
  
  
  function dbltap(target, then, x, y, interval) {
    if (!SyntheticGestures.touchSupported || !target.ownerDocument.createTouch) {
      console.warn('dbltap: touch events not supported; using mouse instead');
      return mousedbltap(target, then, x, y, interval);
    }

    if (x == null)
      x = '50%';
    if (y == null)
      y = '50%';
    var c = coordinates(target, x, y);

    touch(target, 25, [c.x0, c.x0], [c.y0, c.y0], function() {
      
      setTimeout(function() {
        
        touch(target, 25, [c.x0, c.x0], [c.y0, c.y0], then);
      }, interval || 50);
    });
  }

  
  
  function swipe(target, x1, y1, x2, y2, duration, then) {
    if (!SyntheticGestures.touchSupported) {
      console.warn('swipe: touch events not supported; using mouse instead');
      return mouseswipe(target, x1, y1, x2, y2, duration, then);
    }

    var c = coordinates(target, x1, y1, x2, y2);
    touch(target, duration || 200, [c.x0, c.x1], [c.y0, c.y1], then);
  }

  
  
  function hold(target, holdtime, x1, y1, x2, y2, movetime, then) {
    if (!SyntheticGestures.touchSupported || !target.ownerDocument.createTouch) {
      console.warn('hold: touch events not supported; using mouse instead');
      return mousehold(target, holdtime, x1, y1, x2, y2, movetime, then);
    }

    if (!movetime)
      movetime = 200;

    var c = coordinates(target, x1, y1, x2, y2);

    touch(target, holdtime + movetime,
          function(t) { 
            if (t < holdtime)
              return c.x0;
            else
              return c.x0 + (t - holdtime) / movetime * (c.x1 - c.x0);
          },
          function(t) { 
            if (t < holdtime)
              return c.y0;
            else
              return c.y0 + (t - holdtime) / movetime * (c.y1 - c.y0);
          },
          then);
  }

  
  
  
  function pinch(target, x1, y1, x2, y2, scale, duration, then) {
    if (!SyntheticGestures.touchSupported) {
      console.error('pinch: touch events not supported on this platform');
      return;
    }

    var c1 = coordinates(target, x1, y1);
    var c2 = coordinates(target, x2, y2);
    x1 = c1.x0;
    y1 = c1.y0;
    x2 = c2.x0;
    y2 = c2.y0;

    var xmid = (x1 + x2) / 2;
    var ymid = (y1 + y2) / 2;

    var newx1 = Math.round(xmid + (x1 - xmid) * scale);
    var newy1 = Math.round(ymid + (y1 - ymid) * scale);
    var newx2 = Math.round(xmid + (x2 - xmid) * scale);
    var newy2 = Math.round(ymid + (y2 - ymid) * scale);

    
    
    touch(target, duration, [x1, newx1], [y1, newy1]);

    
    
    
    
    touch(target, duration + 200,
          function(t) {
            if (t < duration / 2)
              return x2 + t * 2 / duration * (newx2 - x2);
            else
              return newx2;
          },
          function(t) {
            if (t < duration / 2)
              return y2 + t * 2 / duration * (newy2 - y2);
            else
              return newy2;
          },
          then);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  function drag(doc, duration, xt, yt, then, detail, button, sendClick) {
    var win = doc.defaultView;
    detail = detail || 1;
    button = button || 0;

    var x = xt;
    if (typeof xt !== 'function') {
      x = function(t) { return xt[0] + t / duration * (xt[1] - xt[0]); };
    }

    var y = yt;
    if (typeof yt !== 'function') {
      y = function(t) { return yt[0] + t / duration * (yt[1] - yt[0]); };
    }

    
    var clientX = Math.round(x(0)), clientY = Math.round(y(0));

    
    var lastX = clientX, lastY = clientY;

    
    mouseEvent('mousedown', clientX, clientY);

    
    var startTime = Date.now();
    setTimeout(nextEvent, EVENT_INTERVAL);

    
    
    function mouseEvent(type, clientX, clientY) {
      
      var target = doc.elementFromPoint(clientX, clientY);
      
      var mousedown = doc.createEvent('MouseEvent');
      
      mousedown.initMouseEvent(type,
                               true, true,    
                               win, detail,   
                               clientX + win.mozInnerScreenX,
                               clientY + win.mozInnerScreenY,
                               clientX, clientY,
                               false, false, false, false, 
                               button, null); 
      
      target.dispatchEvent(mousedown);
    }

    function nextEvent() {
      
      var time = Date.now();
      var dt = time - startTime;

      
      var last = dt + EVENT_INTERVAL / 2 > duration;

      
      if (last)
        dt = duration;

      
      clientX = Math.round(x(dt));
      clientY = Math.round(y(dt));

      
      if (clientX !== lastX || clientY !== lastY) { 
        lastX = clientX;
        lastY = clientY;
        mouseEvent('mousemove', clientX, clientY);
      }

      
      
      if (last) {
        mouseEvent('mouseup', lastX, lastY);
        if (sendClick) {
          mouseEvent('click', clientX, clientY);
        }
        if (then) {
          setTimeout(then, 0);
        }
      }
      else {
        setTimeout(nextEvent, EVENT_INTERVAL);
      }
    }
  }

  
  
  function mousetap(target, then, x, y, t, sendClick) {
    if (x == null)
      x = '50%';
    if (y == null)
      y = '50%';
    var c = coordinates(target, x, y);

    drag(target.ownerDocument, t || 50, [c.x0, c.x0], [c.y0, c.y0], then, null, null, sendClick);
  }

  
  
  
  function mousedbltap(target, then, x, y, interval) {
    if (x == null)
      x = '50%';
    if (y == null)
      y = '50%';
    var c = coordinates(target, x, y);

    drag(target.ownerDocument, 25, [c.x0, c.x0], [c.y0, c.y0], function() {
      
      setTimeout(function() {
        
        drag(target.ownerDocument, 25, [c.x0, c.x0], [c.y0, c.y0], then, 2);
      }, interval || 50);
    });
  }

  
  
  function mouseswipe(target, x1, y1, x2, y2, duration, then) {
    var c = coordinates(target, x1, y1, x2, y2);
    drag(target.ownerDocument, duration || 200,
         [c.x0, c.x1], [c.y0, c.y1], then);
  }

  
  
  
  function mousehold(target, holdtime, x1, y1, x2, y2, movetime, then) {
    if (!movetime)
      movetime = 200;

    var c = coordinates(target, x1, y1, x2, y2);

    drag(target.ownerDocument, holdtime + movetime,
          function(t) { 
            if (t < holdtime)
              return c.x0;
            else
              return c.x0 + (t - holdtime) / movetime * (c.x1 - c.x0);
          },
          function(t) { 
            if (t < holdtime)
              return c.y0;
            else
              return c.y0 + (t - holdtime) / movetime * (c.y1 - c.y0);
          },
          then);
  }

  return {
    touchSupported: true,
    tap: tap,
    mousetap: mousetap,
    dbltap: dbltap,
    mousedbltap: mousedbltap,
    swipe: swipe,
    mouseswipe: mouseswipe,
    hold: hold,
    mousehold: mousehold,
    pinch: pinch 
  };
}());
