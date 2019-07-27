







'use strict';

const Ci = Components.interfaces;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = ['PointerRelay', 'PointerAdapter']; 

Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyModuleGetter(this, 'Utils', 
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Logger', 
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'GestureSettings', 
  'resource://gre/modules/accessibility/Gestures.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'GestureTracker', 
  'resource://gre/modules/accessibility/Gestures.jsm');


const MOUSE_ID = 'mouse';

const SYNTH_ID = -1;

let PointerRelay = { 
  




  get _eventsOfInterest() {
    delete this._eventsOfInterest;

    switch (Utils.widgetToolkit) {
      case 'gonk':
      case 'android':
        this._eventsOfInterest = {
          'touchstart' : true,
          'touchmove' : true,
          'touchend' : true,
          'mousedown' : false,
          'mousemove' : false,
          'mouseup': false,
          'click': false };
        break;

      default:
        
        this._eventsOfInterest = {
          'mousemove' : true,
          'mousedown' : true,
          'mouseup': true,
          'click': false
        };
        if ('ontouchstart' in Utils.win) {
          for (let eventType of ['touchstart', 'touchmove', 'touchend']) {
            this._eventsOfInterest[eventType] = true;
          }
        }
        break;
    }

    return this._eventsOfInterest;
  },

  _eventMap: {
    'touchstart' : 'pointerdown',
    'mousedown' : 'pointerdown',
    'touchmove' : 'pointermove',
    'mousemove' : 'pointermove',
    'touchend' : 'pointerup',
    'mouseup': 'pointerup'
  },

  start: function PointerRelay_start(aOnPointerEvent) {
    Logger.debug('PointerRelay.start');
    this.onPointerEvent = aOnPointerEvent;
    for (let eventType in this._eventsOfInterest) {
      Utils.win.addEventListener(eventType, this, true, true);
    }
  },

  stop: function PointerRelay_stop() {
    Logger.debug('PointerRelay.stop');
    delete this.lastPointerMove;
    delete this.onPointerEvent;
    for (let eventType in this._eventsOfInterest) {
      Utils.win.removeEventListener(eventType, this, true, true);
    }
  },

  handleEvent: function PointerRelay_handleEvent(aEvent) {
    
    if (Utils.MozBuildApp === 'browser' &&
      aEvent.view.top instanceof Ci.nsIDOMChromeWindow) {
      return;
    }
    if (aEvent.mozInputSource === Ci.nsIDOMMouseEvent.MOZ_SOURCE_UNKNOWN ||
        aEvent.isSynthesized) {
      
      return;
    }

    let changedTouches = aEvent.changedTouches || [{
      identifier: MOUSE_ID,
      screenX: aEvent.screenX,
      screenY: aEvent.screenY,
      target: aEvent.target
    }];

    if (Utils.widgetToolkit === 'android' &&
      changedTouches.length === 1 && changedTouches[0].identifier === 1) {
      changedTouches = [{
        identifier: 0,
        screenX: changedTouches[0].screenX + 5,
        screenY: changedTouches[0].screenY + 5,
        target: changedTouches[0].target
      }, changedTouches[0]];
    }

    if (changedTouches.length === 1 &&
        changedTouches[0].identifier === SYNTH_ID) {
      return;
    }

    aEvent.preventDefault();
    aEvent.stopImmediatePropagation();

    let type = aEvent.type;
    if (!this._eventsOfInterest[type]) {
      return;
    }
    let pointerType = this._eventMap[type];
    this.onPointerEvent({
      type: pointerType,
      points: Array.prototype.map.call(changedTouches,
        function mapTouch(aTouch) {
          return {
            identifier: aTouch.identifier,
            x: aTouch.screenX,
            y: aTouch.screenY
          };
        }
      )
    });
  }
};

this.PointerAdapter = { 
  start: function PointerAdapter_start() {
    Logger.debug('PointerAdapter.start');
    GestureTracker.reset();
    PointerRelay.start(this.handleEvent);
  },

  stop: function PointerAdapter_stop() {
    Logger.debug('PointerAdapter.stop');
    PointerRelay.stop();
    GestureTracker.reset();
  },

  handleEvent: function PointerAdapter_handleEvent(aDetail) {
    let timeStamp = Date.now();
    GestureTracker.handle(aDetail, timeStamp);
  }
};
