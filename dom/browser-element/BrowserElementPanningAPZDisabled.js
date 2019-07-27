






"use strict";
dump("############################### browserElementPanningAPZDisabled.js loaded\n");

let { classes: Cc, interfaces: Ci, results: Cr, utils: Cu }  = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Geometry.jsm");

var global = this;

const ContentPanningAPZDisabled = {
  
  watchedEventsType: '',

  
  
  hybridEvents: false,

  init: function cp_init() {
    this._setupListenersForPanning();
  },

  _setupListenersForPanning: function cp_setupListenersForPanning() {
    let events;

    if (content.TouchEvent) {
      events = ['touchstart', 'touchend', 'touchmove'];
      this.watchedEventsType = 'touch';
#ifdef MOZ_WIDGET_GONK
      
      
      
      let appInfo = Cc["@mozilla.org/xre/app-info;1"];
      let isParentProcess =
        !appInfo || appInfo.getService(Ci.nsIXULRuntime)
                           .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
      this.hybridEvents = isParentProcess;
#endif
    } else {
      
      events = ['mousedown', 'mouseup', 'mousemove'];
      this.watchedEventsType = 'mouse';
    }

    let els = Cc["@mozilla.org/eventlistenerservice;1"]
                .getService(Ci.nsIEventListenerService);

    events.forEach(function(type) {
      
      
      els.addSystemEventListener(global, type,
                                 this.handleEvent.bind(this),
                                  false);
    }.bind(this));
  },

  handleEvent: function cp_handleEvent(evt) {
    
    
    if (evt.target instanceof Ci.nsIMozBrowserFrame) {
      return;
    }

    
    
    
    
    
    
    
    let targetWindow = evt.target.ownerDocument.defaultView;
    let frameElement = targetWindow.frameElement;
    while (frameElement) {
      targetWindow = frameElement.ownerDocument.defaultView;
      frameElement = targetWindow.frameElement;
    }

    if (content !== targetWindow) {
      return;
    }

    if (evt.defaultPrevented || evt.multipleActionsPrevented) {
      
      if(evt.type === 'touchend' || evt.type === 'mouseup') {
        if (this.dragging &&
            (this.watchedEventsType === 'mouse' ||
             this.findPrimaryPointer(evt.changedTouches))) {
          this._finishPanning();
        }
      }
      return;
    }

    switch (evt.type) {
      case 'mousedown':
      case 'touchstart':
        this.onTouchStart(evt);
        break;
      case 'mousemove':
      case 'touchmove':
        this.onTouchMove(evt);
        break;
      case 'mouseup':
      case 'touchend':
        this.onTouchEnd(evt);
        break;
      case 'click':
        evt.stopPropagation();
        evt.preventDefault();

        let target = evt.target;
        let view = target.ownerDocument ? target.ownerDocument.defaultView
                                        : target;
        view.removeEventListener('click', this, true, true);
        break;
    }
  },

  position: new Point(0 , 0),

  findPrimaryPointer: function cp_findPrimaryPointer(touches) {
    if (!('primaryPointerId' in this))
      return null;

    for (let i = 0; i < touches.length; i++) {
      if (touches[i].identifier === this.primaryPointerId) {
        return touches[i];
      }
    }
    return null;
  },

  onTouchStart: function cp_onTouchStart(evt) {
    let screenX, screenY;
    if (this.watchedEventsType == 'touch') {
      if ('primaryPointerId' in this || evt.touches.length >= 2) {
        this._resetActive();
        return;
      }

      let firstTouch = evt.changedTouches[0];
      this.primaryPointerId = firstTouch.identifier;
      this.pointerDownTarget = firstTouch.target;
      screenX = firstTouch.screenX;
      screenY = firstTouch.screenY;
    } else {
      this.pointerDownTarget = evt.target;
      screenX = evt.screenX;
      screenY = evt.screenY;
    }
    this.dragging = true;
    this.panning = false;

    let oldTarget = this.target;
    [this.target, this.scrollCallback] = this.getPannable(this.pointerDownTarget);

    
    
    
    
    if (this.pointerDownTarget !== null) {
      
      
      
      if (this.target === null) {
        this.notify(this._activationTimer);
      } else {
        this._activationTimer.initWithCallback(this,
                                               this._activationDelayMs,
                                               Ci.nsITimer.TYPE_ONE_SHOT);
      }
    }

    
    
    
    
    this.preventNextClick = false;
    if (KineticPanning.active) {
      KineticPanning.stop();

      if (oldTarget && oldTarget == this.target)
        this.preventNextClick = true;
    }

    this.position.set(screenX, screenY);
    KineticPanning.reset();
    KineticPanning.record(new Point(0, 0), evt.timeStamp);

    
    
    if ((this.panning || this.preventNextClick)) {
      evt.preventDefault();
    }
  },

  onTouchEnd: function cp_onTouchEnd(evt) {
    let touch = null;
    if (!this.dragging ||
        (this.watchedEventsType == 'touch' &&
         !(touch = this.findPrimaryPointer(evt.changedTouches)))) {
      return;
    }

    
    
    
    
    
    
    
    
    
    let click = (this.watchedEventsType == 'mouse') ?
      evt.detail : !KineticPanning.isPan();
    
    
    
    if (this.hybridEvents) {
      let target =
        content.document.elementFromPoint(touch.clientX, touch.clientY);
      click |= (target === this.pointerDownTarget);
    }

    if (this.target && click && (this.panning || this.preventNextClick)) {
      if (this.hybridEvents) {
        let target = this.target;
        let view = target.ownerDocument ? target.ownerDocument.defaultView
                                        : target;
        view.addEventListener('click', this, true, true);
      } else {
        
        evt.preventDefault();
      }
    } else if (this.target && click && !this.panning) {
      this.notify(this._activationTimer);
    }

    this._finishPanning();

    
    this.pointerDownTarget = null;
  },

  onTouchMove: function cp_onTouchMove(evt) {
    if (!this.dragging)
      return;

    let screenX, screenY;
    if (this.watchedEventsType == 'touch') {
      let primaryTouch = this.findPrimaryPointer(evt.changedTouches);
      if (evt.touches.length > 1 || !primaryTouch)
        return;
      screenX = primaryTouch.screenX;
      screenY = primaryTouch.screenY;
    } else {
      screenX = evt.screenX;
      screenY = evt.screenY;
    }

    let current = this.position;
    let delta = new Point(screenX - current.x, screenY - current.y);
    current.set(screenX, screenY);

    KineticPanning.record(delta, evt.timeStamp);

    let isPan = KineticPanning.isPan();

    
    
    if (!this.panning && isPan) {
      this._resetActive();
    }

    
    if (!this.scrollCallback) {
      return;
    }

    
    this.scrollCallback(delta.scale(-1));

    if (!this.panning && isPan) {
      this.panning = true;
      this._activationTimer.cancel();
    }

    if (this.panning) {
      
      
      evt.stopPropagation();
      evt.preventDefault();
    }
  },

  
  notify: function cp_notify(timer) {
    this._setActive(this.pointerDownTarget);
  },

  onKineticBegin: function cp_onKineticBegin(evt) {
  },

  onKineticPan: function cp_onKineticPan(delta) {
    return !this.scrollCallback(delta);
  },

  onKineticEnd: function cp_onKineticEnd() {
    if (!this.dragging)
      this.scrollCallback = null;
  },

  getPannable: function cp_getPannable(node) {
    let pannableNode = this._findPannable(node);
    if (pannableNode) {
      return [pannableNode, this._generateCallback(pannableNode)];
    }

    return [null, null];
  },

  _findPannable: function cp_findPannable(node) {
    if (!(node instanceof Ci.nsIDOMHTMLElement) || node.tagName == 'HTML') {
      return null;
    }

    let nodeContent = node.ownerDocument.defaultView;
    while (!(node instanceof Ci.nsIDOMHTMLBodyElement)) {
      let style = nodeContent.getComputedStyle(node, null);

      let overflow = [style.getPropertyValue('overflow'),
                      style.getPropertyValue('overflow-x'),
                      style.getPropertyValue('overflow-y')];

      let rect = node.getBoundingClientRect();
      let isAuto = (overflow.indexOf('auto') != -1 &&
                   (rect.height < node.scrollHeight ||
                    rect.width < node.scrollWidth));

      let isScroll = (overflow.indexOf('scroll') != -1);

      let isScrollableTextarea = (node.tagName == 'TEXTAREA' &&
          (node.scrollHeight > node.clientHeight ||
           node.scrollWidth > node.clientWidth ||
           ('scrollLeftMax' in node && node.scrollLeftMax > 0) ||
           ('scrollTopMax' in node && node.scrollTopMax > 0)));
      if (isScroll || isAuto || isScrollableTextarea) {
        return node;
      }

      node = node.parentNode;
    }

    if (nodeContent.scrollMaxX || nodeContent.scrollMaxY) {
      return nodeContent;
    }

    if (nodeContent.frameElement) {
      return this._findPannable(nodeContent.frameElement);
    }

    return null;
  },

  _generateCallback: function cp_generateCallback(root) {
    let firstScroll = true;
    let target;
    let current;
    let win, doc, htmlNode, bodyNode;

    function doScroll(node, delta) {
      if (node instanceof Ci.nsIDOMHTMLElement) {
        return node.scrollByNoFlush(delta.x, delta.y);
      } else if (node instanceof Ci.nsIDOMWindow) {
        win = node;
        doc = win.document;

        
        
        if (doc instanceof Ci.nsIDOMHTMLDocument) {
          htmlNode = doc.documentElement;
          bodyNode = doc.body;
          if (win.getComputedStyle(htmlNode, null).overflowX == "hidden" ||
              win.getComputedStyle(bodyNode, null).overflowX == "hidden") {
            delta.x = 0;
          }
          if (win.getComputedStyle(htmlNode, null).overflowY == "hidden" ||
              win.getComputedStyle(bodyNode, null).overflowY == "hidden") {
            delta.y = 0;
          }
        }
        let oldX = node.scrollX;
        let oldY = node.scrollY;
        node.scrollBy(delta.x, delta.y);
        return (node.scrollX != oldX || node.scrollY != oldY);
      }
      
      
      return false;
    }

    function targetParent(node) {
      return node.parentNode || node.frameElement || null;
    }

    function scroll(delta) {
      current = root;
      firstScroll = true;
      while (current) {
        if (doScroll(current, delta)) {
          firstScroll = false;
          return true;
        }

        
        
        if (!firstScroll) {
          return false;
        }

        current = ContentPanningAPZDisabled._findPannable(targetParent(current));
      }

      
      return false;
    }
    return scroll;
  },

  get _activationTimer() {
    delete this._activationTimer;
    return this._activationTimer = Cc["@mozilla.org/timer;1"]
                                     .createInstance(Ci.nsITimer);
  },

  get _activationDelayMs() {
    let delay = Services.prefs.getIntPref('ui.touch_activation.delay_ms');
    delete this._activationDelayMs;
    return this._activationDelayMs = delay;
  },

  get _domUtils() {
    delete this._domUtils;
    return this._domUtils = Cc['@mozilla.org/inspector/dom-utils;1']
                              .getService(Ci.inIDOMUtils);
  },

  _resetActive: function cp_resetActive() {
    let elt = this.pointerDownTarget || this.target;
    let root = elt.ownerDocument || elt.document;
    this._setActive(root.documentElement);
  },

  _setActive: function cp_setActive(elt) {
    const kStateActive = 0x00000001;
    this._domUtils.setContentState(elt, kStateActive);
  },

  _finishPanning: function() {
    this.dragging = false;
    delete this.primaryPointerId;
    this._activationTimer.cancel();

    
    if (this.panning) {
      KineticPanning.start(this);
    }
  },
};


const kMinVelocity = 0.2;
const kMaxVelocity = 6;


const kExponentialC = 1000;
const kPolynomialC = 100 / 1000000;




const kUpdateInterval = 16;



const kSamples = 5;

const KineticPanning = {
  _position: new Point(0, 0),
  _velocity: new Point(0, 0),
  _acceleration: new Point(0, 0),

  get active() {
    return this.target !== null;
  },

  target: null,
  start: function kp_start(target) {
    this.target = target;

    
    let momentums = this.momentums;
    let flick = momentums[momentums.length - 1].time - momentums[0].time < 300;

    let distance = new Point(0, 0);
    momentums.forEach(function(momentum) {
      distance.add(momentum.dx, momentum.dy);
    });

    function clampFromZero(x, min, max) {
      if (x >= 0)
        return Math.max(min, Math.min(max, x));
      return Math.min(-min, Math.max(-max, x));
    }

    let elapsed = momentums[momentums.length - 1].time - momentums[0].time;
    let velocityX = clampFromZero(distance.x / elapsed, 0, kMaxVelocity);
    let velocityY = clampFromZero(distance.y / elapsed, 0, kMaxVelocity);

    let velocity = this._velocity;
    if (flick) {
      
      
      
      
      
      velocity.set(velocityX, velocityY);
    } else {
      velocity.set(Math.abs(velocityX) < kMinVelocity ? 0 : velocityX,
                   Math.abs(velocityY) < kMinVelocity ? 0 : velocityY);
    }
    this.momentums = [];

    
    function sign(x) {
      return x ? (x > 0 ? 1 : -1) : 0;
    }

    this._acceleration.set(velocity.clone().map(sign).scale(-kPolynomialC));

    
    this._position.set(0, 0);

    this._startAnimation();

    this.target.onKineticBegin();
  },

  stop: function kp_stop() {
    this.reset();

    if (!this.target)
      return;

    this.target.onKineticEnd();
    this.target = null;
  },

  reset: function kp_reset() {
    this.momentums = [];
    this.distance.set(0, 0);
  },

  momentums: [],
  record: function kp_record(delta, timestamp) {
    this.momentums.push({ 'time': this._getTime(timestamp),
                          'dx' : delta.x, 'dy' : delta.y });

    
    if (this.momentums.length > kSamples) {
      this.momentums.shift();
    }

    this.distance.add(delta.x, delta.y);
  },

  _getTime: function kp_getTime(time) {
    
    
    
    if (time > Date.now()) {
      return Math.floor(time / 1000);
    } else {
      return time;
    }
  },

  get threshold() {
    let dpi = content.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils)
                     .displayDPI;

    let threshold = Services.prefs.getIntPref('ui.dragThresholdX') / 240 * dpi;

    delete this.threshold;
    return this.threshold = threshold;
  },

  distance: new Point(0, 0),
  isPan: function cp_isPan() {
    return (Math.abs(this.distance.x) > this.threshold ||
            Math.abs(this.distance.y) > this.threshold);
  },

  _startAnimation: function kp_startAnimation() {
    let c = kExponentialC;
    function getNextPosition(position, v, a, t) {
      
      
      
      
      
      
      
      position.set(v.x * Math.exp(-t / c) * -c + a.x * t * t + v.x * c,
                   v.y * Math.exp(-t / c) * -c + a.y * t * t + v.y * c);
    }

    let startTime = content.mozAnimationStartTime;
    let elapsedTime = 0, targetedTime = 0, averageTime = 0;

    let velocity = this._velocity;
    let acceleration = this._acceleration;

    let position = this._position;
    let nextPosition = new Point(0, 0);
    let delta = new Point(0, 0);

    let callback = (function(timestamp) {
      if (!this.target)
        return;

      
      
      
      
      elapsedTime = timestamp - startTime;
      targetedTime += kUpdateInterval;
      averageTime = (targetedTime + elapsedTime) / 2;

      
      getNextPosition(nextPosition, velocity, acceleration, averageTime);
      delta.set(Math.round(nextPosition.x - position.x),
                Math.round(nextPosition.y - position.y));

      
      if (delta.x * acceleration.x > 0)
        delta.x = position.x = velocity.x = acceleration.x = 0;

      if (delta.y * acceleration.y > 0)
        delta.y = position.y = velocity.y = acceleration.y = 0;

      if (velocity.equals(0, 0) || delta.equals(0, 0)) {
        this.stop();
        return;
      }

      position.add(delta);
      if (this.target.onKineticPan(delta.scale(-1))) {
        this.stop();
        return;
      }

      content.mozRequestAnimationFrame(callback);
    }).bind(this);

    content.mozRequestAnimationFrame(callback);
  }
};
