




let {CC, Cc, Ci, Cu, Cr} = require('chrome');

Cu.import('resource://gre/modules/Services.jsm');

let handlerCount = 0;

let orig_w3c_touch_events = Services.prefs.getIntPref('dom.w3c_touch_events.enabled');



function TouchEventHandler (window) {
  let contextMenuTimeout = 0;

  
  
  let ignoreEvents = false;

  let threshold = 25;
  try {
    threshold = Services.prefs.getIntPref('ui.dragThresholdX');
  } catch(e) {}

  let delay = 500;
  try {
    delay = Services.prefs.getIntPref('ui.click_hold_context_menus.delay');
  } catch(e) {}

  let TouchEventHandler = {
    enabled: false,
    events: ['mousedown', 'mousemove', 'mouseup', 'click'],
    start: function teh_start() {
      let isReloadNeeded = Services.prefs.getIntPref('dom.w3c_touch_events.enabled') != 1;
      handlerCount++;
      Services.prefs.setIntPref('dom.w3c_touch_events.enabled', 1);
      this.enabled = true;
      this.events.forEach((function(evt) {
        window.addEventListener(evt, this, true);
      }).bind(this));
      return isReloadNeeded;
    },
    stop: function teh_stop() {
      handlerCount--;
      if (handlerCount == 0)
        Services.prefs.setIntPref('dom.w3c_touch_events.enabled', orig_w3c_touch_events);
      this.enabled = false;
      this.events.forEach((function(evt) {
        window.removeEventListener(evt, this, true);
      }).bind(this));
    },
    handleEvent: function teh_handleEvent(evt) {
      if (evt.button || ignoreEvents ||
          evt.mozInputSource == Ci.nsIDOMMouseEvent.MOZ_SOURCE_UNKNOWN)
        return;

      
      
      
      let content = this.getContent(evt.target);
      let isSystemWindow = content.location.toString().indexOf("system.gaiamobile.org") != -1;

      let eventTarget = this.target;
      let type = '';
      switch (evt.type) {
        case 'mousedown':
          this.target = evt.target;

          contextMenuTimeout =
            this.sendContextMenu(evt.target, evt.pageX, evt.pageY, delay);

          this.cancelClick = false;
          this.startX = evt.pageX;
          this.startY = evt.pageY;

          
          
          evt.target.setCapture(false);

          type = 'touchstart';
          break;

        case 'mousemove':
          if (!eventTarget)
            return;

          if (!this.cancelClick) {
            if (Math.abs(this.startX - evt.pageX) > threshold ||
                Math.abs(this.startY - evt.pageY) > threshold) {
              this.cancelClick = true;
              content.clearTimeout(contextMenuTimeout);
            }
          }

          type = 'touchmove';
          break;

        case 'mouseup':
          if (!eventTarget)
            return;
          this.target = null;

          content.clearTimeout(contextMenuTimeout);
          type = 'touchend';
          break;

        case 'click':
          
          
          evt.preventDefault();
          evt.stopImmediatePropagation();

          if (this.cancelClick)
            return;

          ignoreEvents = true;
          content.setTimeout(function dispatchMouseEvents(self) {
            self.fireMouseEvent('mousedown', evt);
            self.fireMouseEvent('mousemove', evt);
            self.fireMouseEvent('mouseup', evt);
            ignoreEvents = false;
         }, 0, this);

          return;
      }

      let target = eventTarget || this.target;
      if (target && type) {
        this.sendTouchEvent(evt, target, type);
      }

      if (!isSystemWindow) {
        evt.preventDefault();
        evt.stopImmediatePropagation();
      }
    },
    fireMouseEvent: function teh_fireMouseEvent(type, evt)  {
      let content = this.getContent(evt.target);
      var utils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils);
      utils.sendMouseEvent(type, evt.clientX, evt.clientY, 0, 1, 0, true);
    },
    sendContextMenu: function teh_sendContextMenu(target, x, y, delay) {
      let doc = target.ownerDocument;
      let evt = doc.createEvent('MouseEvent');
      evt.initMouseEvent('contextmenu', true, true, doc.defaultView,
                         0, x, y, x, y, false, false, false, false,
                         0, null);

      let content = this.getContent(target);
      let timeout = content.setTimeout((function contextMenu() {
        target.dispatchEvent(evt);
        this.cancelClick = true;
      }).bind(this), delay);

      return timeout;
    },
    sendTouchEvent: function teh_sendTouchEvent(evt, target, name) {
      let document = target.ownerDocument;
      let content = this.getContent(target);

      let touchEvent = document.createEvent('touchevent');
      let point = document.createTouch(content, target, 0,
                                       evt.pageX, evt.pageY,
                                       evt.screenX, evt.screenY,
                                       evt.clientX, evt.clientY,
                                       1, 1, 0, 0);
      let touches = document.createTouchList(point);
      let targetTouches = touches;
      let changedTouches = touches;
      touchEvent.initTouchEvent(name, true, true, content, 0,
                                false, false, false, false,
                                touches, targetTouches, changedTouches);
      target.dispatchEvent(touchEvent);
      return touchEvent;
    },
    getContent: function teh_getContent(target) {
      let win = target.ownerDocument.defaultView;
      let docShell = win.QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIWebNavigation)
                    .QueryInterface(Ci.nsIDocShellTreeItem);
      let topDocShell = docShell.sameTypeRootTreeItem;
      topDocShell.QueryInterface(Ci.nsIDocShell);
      let top = topDocShell.contentViewer.DOMDocument.defaultView;
      return top;
    }
  };

  return TouchEventHandler;
}

exports.TouchEventHandler = TouchEventHandler;
