






this.ActionChain = function (utils, checkForInterrupted) {
  
  this.nextTouchId = 1000;
  
  this.touchIds = {};
  
  this.lastCoordinates = null;
  this.isTap = false;
  this.scrolling = false;
  
  this.mouseEventsOnly = false;
  this.checkTimer = Components.classes["@mozilla.org/timer;1"]
                              .createInstance(Components.interfaces.nsITimer);

  
  this.onSuccess = null;
  this.onError = null;
  if (typeof checkForInterrupted == "function") {
    this.checkForInterrupted = checkForInterrupted;
  } else {
    this.checkForInterrupted = () => {};
  }

  
  this.inputSource = null;

  
  this.utils = utils;
}

ActionChain.prototype = {

  dispatchActions: function (args, touchId, frame, elementManager, callbacks,
                             touchProvider) {
    
    
    if (touchProvider) {
      this.touchProvider = touchProvider;
    }

    this.elementManager = elementManager;
    let commandArray = elementManager.convertWrappedArguments(args, frame);
    let {onSuccess, onError} = callbacks;
    this.onSuccess = onSuccess;
    this.onError = onError;
    this.frame = frame;

    if (touchId == null) {
      touchId = this.nextTouchId++;
    }

    if (!frame.document.createTouch) {
      this.mouseEventsOnly = true;
    }

    let keyModifiers = {
      shiftKey: false,
      ctrlKey: false,
      altKey: false,
      metaKey: false
    };

    try {
      this.actions(commandArray, touchId, 0, keyModifiers);
    } catch (e) {
      this.onError(e.message, e.code, e.stack);
      this.resetValues();
    }
  },

  







  emitMouseEvent: function (doc, type, elClientX, elClientY, button, clickCount, modifiers) {
    if (!this.checkForInterrupted()) {
      let loggingInfo = "emitting Mouse event of type " + type +
        " at coordinates (" + elClientX + ", " + elClientY +
        ") relative to the viewport\n" +
        " button: " + button + "\n" +
        " clickCount: " + clickCount + "\n";
      dump(Date.now() + " Marionette: " + loggingInfo);
      let win = doc.defaultView;
      let domUtils = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
        .getInterface(Components.interfaces.nsIDOMWindowUtils);
      let mods;
      if (typeof modifiers != "undefined") {
        mods = this.utils._parseModifiers(modifiers);
      } else {
        mods = 0;
      }
      domUtils.sendMouseEvent(type, elClientX, elClientY, button || 0, clickCount || 1,
                              mods, false, 0, this.inputSource);
    }
  },

  


  resetValues: function () {
    this.onSuccess = null;
    this.onError = null;
    this.frame = null;
    this.elementManager = null;
    this.touchProvider = null;
    this.mouseEventsOnly = false;
  },

  




  actions: function (chain, touchId, i, keyModifiers) {

    if (i == chain.length) {
      this.onSuccess({value: touchId});
      this.resetValues();
      return;
    }

    let pack = chain[i];
    let command = pack[0];
    let el;
    let c;
    i++;

    if (['press', 'wait', 'keyDown', 'keyUp', 'click'].indexOf(command) == -1) {
      
      if (!(touchId in this.touchIds) && !this.mouseEventsOnly) {
        this.onError("Element has not been pressed", 500, null);
        this.resetValues();
        return;
      }
    }

    switch(command) {
    case 'keyDown':
      this.utils.sendKeyDown(pack[1], keyModifiers, this.frame);
      this.actions(chain, touchId, i, keyModifiers);
      break;
    case 'keyUp':
      this.utils.sendKeyUp(pack[1], keyModifiers, this.frame);
      this.actions(chain, touchId, i, keyModifiers);
      break;
    case 'click':
      el = this.elementManager.getKnownElement(pack[1], this.frame);
      let button = pack[2];
      let clickCount = pack[3];
      c = this.coordinates(el, null, null);
      this.mouseTap(el.ownerDocument, c.x, c.y, button, clickCount,
                    keyModifiers);
      if (button == 2) {
        this.emitMouseEvent(el.ownerDocument, 'contextmenu', c.x, c.y,
                            button, clickCount, keyModifiers);
      }
      this.actions(chain, touchId, i, keyModifiers);
      break;
    case 'press':
      if (this.lastCoordinates) {
        this.generateEvents('cancel', this.lastCoordinates[0], this.lastCoordinates[1],
                            touchId, null, keyModifiers);
        this.onError("Invalid Command: press cannot follow an active touch event", 500, null);
        this.resetValues();
        return;
      }
      
      if ((i != chain.length) && (chain[i][0].indexOf('move') !== -1)) {
        this.scrolling = true;
      }
      el = this.elementManager.getKnownElement(pack[1], this.frame);
      c = this.coordinates(el, pack[2], pack[3]);
      touchId = this.generateEvents('press', c.x, c.y, null, el, keyModifiers);
      this.actions(chain, touchId, i, keyModifiers);
      break;
    case 'release':
      this.generateEvents('release', this.lastCoordinates[0], this.lastCoordinates[1],
                          touchId, null, keyModifiers);
      this.actions(chain, null, i, keyModifiers);
      this.scrolling =  false;
      break;
    case 'move':
      el = this.elementManager.getKnownElement(pack[1], this.frame);
      c = this.coordinates(el);
      this.generateEvents('move', c.x, c.y, touchId, null, keyModifiers);
      this.actions(chain, touchId, i, keyModifiers);
      break;
    case 'moveByOffset':
      this.generateEvents('move', this.lastCoordinates[0] + pack[1],
                          this.lastCoordinates[1] + pack[2],
                          touchId, null, keyModifiers);
      this.actions(chain, touchId, i, keyModifiers);
      break;
    case 'wait':
      if (pack[1] != null ) {
        let time = pack[1]*1000;
        
        let standard = 750;
        try {
          standard = Services.prefs.getIntPref("ui.click_hold_context_menus.delay");
        }
        catch (e){}
        if (time >= standard && this.isTap) {
          chain.splice(i, 0, ['longPress'], ['wait', (time-standard)/1000]);
          time = standard;
        }
        this.checkTimer.initWithCallback(() => {
          this.actions(chain, touchId, i, keyModifiers);
        }, time, Components.interfaces.nsITimer.TYPE_ONE_SHOT);
      }
      else {
        this.actions(chain, touchId, i, keyModifiers);
      }
      break;
    case 'cancel':
      this.generateEvents('cancel', this.lastCoordinates[0], this.lastCoordinates[1],
                          touchId, null, keyModifiers);
      this.actions(chain, touchId, i, keyModifiers);
      this.scrolling = false;
      break;
    case 'longPress':
      this.generateEvents('contextmenu', this.lastCoordinates[0], this.lastCoordinates[1],
                          touchId, null, keyModifiers);
      this.actions(chain, touchId, i, keyModifiers);
      break;
    }
  },

  





  coordinates: function (target, x, y) {
    let box = target.getBoundingClientRect();
    if (x == null) {
      x = box.width / 2;
    }
    if (y == null) {
      y = box.height / 2;
    }
    let coords = {};
    coords.x = box.left + x;
    coords.y = box.top + y;
    return coords;
  },

  



  getCoordinateInfo: function (el, corx, cory) {
    let win = el.ownerDocument.defaultView;
    return [ corx, 
             cory, 
             corx + win.pageXOffset, 
             cory + win.pageYOffset, 
             corx + win.mozInnerScreenX, 
             cory + win.mozInnerScreenY 
           ];
  },

  
  generateEvents: function (type, x, y, touchId, target, keyModifiers) {
    this.lastCoordinates = [x, y];
    let doc = this.frame.document;
    switch (type) {
    case 'tap':
      if (this.mouseEventsOnly) {
        this.mouseTap(touch.target.ownerDocument, touch.clientX, touch.clientY,
                      null, null, keyModifiers);
      } else {
        touchId = this.nextTouchId++;
        let touch = this.touchProvider.createATouch(target, x, y, touchId);
        this.touchProvider.emitTouchEvent('touchstart', touch);
        this.touchProvider.emitTouchEvent('touchend', touch);
        this.mouseTap(touch.target.ownerDocument, touch.clientX, touch.clientY,
                      null, null, keyModifiers);
      }
      this.lastCoordinates = null;
      break;
    case 'press':
      this.isTap = true;
      if (this.mouseEventsOnly) {
        this.emitMouseEvent(doc, 'mousemove', x, y, null, null, keyModifiers);
        this.emitMouseEvent(doc, 'mousedown', x, y, null, null, keyModifiers);
      }
      else {
        touchId = this.nextTouchId++;
        let touch = this.touchProvider.createATouch(target, x, y, touchId);
        this.touchProvider.emitTouchEvent('touchstart', touch);
        this.touchIds[touchId] = touch;
        return touchId;
      }
      break;
    case 'release':
      if (this.mouseEventsOnly) {
        let [x, y] = this.lastCoordinates;
        this.emitMouseEvent(doc, 'mouseup', x, y,
                            null, null, keyModifiers);
      }
      else {
        let touch = this.touchIds[touchId];
        let [x, y] = this.lastCoordinates;
        touch = this.touchProvider.createATouch(touch.target, x, y, touchId);
        this.touchProvider.emitTouchEvent('touchend', touch);
        if (this.isTap) {
          this.mouseTap(touch.target.ownerDocument, touch.clientX, touch.clientY,
                        null, null, keyModifiers);
        }
        delete this.touchIds[touchId];
      }
      this.isTap = false;
      this.lastCoordinates = null;
      break;
    case 'cancel':
      this.isTap = false;
      if (this.mouseEventsOnly) {
        let [x, y] = this.lastCoordinates;
        this.emitMouseEvent(doc, 'mouseup', x, y,
                            null, null, keyModifiers);
      }
      else {
        this.touchProvider.emitTouchEvent('touchcancel', this.touchIds[touchId]);
        delete this.touchIds[touchId];
      }
      this.lastCoordinates = null;
      break;
    case 'move':
      this.isTap = false;
      if (this.mouseEventsOnly) {
        this.emitMouseEvent(doc, 'mousemove', x, y, null, null, keyModifiers);
      }
      else {
        let touch = this.touchProvider.createATouch(this.touchIds[touchId].target,
                                                    x, y, touchId);
        this.touchIds[touchId] = touch;
        this.touchProvider.emitTouchEvent('touchmove', touch);
      }
      break;
    case 'contextmenu':
      this.isTap = false;
      let event = this.frame.document.createEvent('MouseEvents');
      if (this.mouseEventsOnly) {
        target = doc.elementFromPoint(this.lastCoordinates[0], this.lastCoordinates[1]);
      }
      else {
        target = this.touchIds[touchId].target;
      }
      let [ clientX, clientY,
            pageX, pageY,
            screenX, screenY ] = this.getCoordinateInfo(target, x, y);
      event.initMouseEvent('contextmenu', true, true,
                           target.ownerDocument.defaultView, 1,
                           screenX, screenY, clientX, clientY,
                           false, false, false, false, 0, null);
      target.dispatchEvent(event);
      break;
    default:
      throw {message:"Unknown event type: " + type, code: 500, stack:null};
    }
    this.checkForInterrupted();
  },

  mouseTap: function (doc, x, y, button, clickCount, keyModifiers) {
    this.emitMouseEvent(doc, 'mousemove', x, y, button, clickCount, keyModifiers);
    this.emitMouseEvent(doc, 'mousedown', x, y, button, clickCount, keyModifiers);
    this.emitMouseEvent(doc, 'mouseup', x, y, button, clickCount, keyModifiers);
  },
}
