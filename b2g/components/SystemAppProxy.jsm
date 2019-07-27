



'use strict';

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

this.EXPORTED_SYMBOLS = ['SystemAppProxy'];

let SystemAppProxy = {
  _frame: null,
  _isReady: false,
  _pendingEvents: [],
  _pendingListeners: [],

  
  registerFrame: function (frame) {
    this._isReady = false;
    this._frame = frame;

    
    this._pendingListeners
        .forEach((args) =>
                 this.addEventListener.apply(this, args));
    this._pendingListeners = [];
  },

  
  setIsReady: function () {
    if (this._isReady) {
      Cu.reportError('SystemApp has already been declared as being ready.');
    }
    this._isReady = true;

    
    this._pendingEvents
        .forEach(([type, details]) =>
                 this._sendCustomEvent(type, details));
    this._pendingEvents = [];
  },

  














  _sendCustomEvent: function systemApp_sendCustomEvent(type,
                                                       details,
                                                       noPending,
                                                       target) {
    let content = this._frame ? this._frame.contentWindow : null;

    
    
    if (!content || (!this._isReady && !noPending)) {
      this._pendingEvents.push([type, details]);
      return null;
    }

    let event = content.document.createEvent('CustomEvent');

    let payload;
    
    
    if ('__exposedProps__' in details) {
      payload = details;
    } else {
      payload = details ? Cu.cloneInto(details, content) : {};
    }

    event.initCustomEvent(type, true, false, payload);
    (target || content).dispatchEvent(event);

    return event;
  },

  
  dispatchEvent: function systemApp_dispatchEvent(details, target) {
    return this._sendCustomEvent('mozChromeEvent', details, false, target);
  },

  dispatchKeyboardEvent: function systemApp_dispatchKeyboardEvent(type, details) {
    let content = this._frame ? this._frame.contentWindow : null;
    let e = new content.KeyboardEvent(type, details);
    content.dispatchEvent(e);
  },

  
  addEventListener: function systemApp_addEventListener() {
    let content = this._frame ? this._frame.contentWindow : null;
    if (!content) {
      this._pendingListeners.push(arguments);
      return false;
    }

    content.addEventListener.apply(content, arguments);
    return true;
  },

  removeEventListener: function systemApp_removeEventListener(name, listener) {
    let content = this._frame ? this._frame.contentWindow : null;
    if (content) {
      content.removeEventListener.apply(content, arguments);
    } else {
      this._pendingListeners = this._pendingListeners.filter(
        args => {
          return args[0] != name || args[1] != listener;
        });
    }
  },

  getFrames: function systemApp_getFrames() {
    let systemAppFrame = this._frame;
    if (!systemAppFrame) {
      return [];
    }
    let list = [systemAppFrame];
    let frames = systemAppFrame.contentDocument.querySelectorAll('iframe');
    for (let i = 0; i < frames.length; i++) {
      list.push(frames[i]);
    }
    return list;
  }
};
this.SystemAppProxy = SystemAppProxy;

