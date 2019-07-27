










































'use strict';

const Ci = Components.interfaces;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = ['GestureSettings', 'GestureTracker']; 

Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyModuleGetter(this, 'Utils', 
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Logger', 
  'resource://gre/modules/accessibility/Utils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'setTimeout', 
  'resource://gre/modules/Timer.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'clearTimeout', 
  'resource://gre/modules/Timer.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Promise', 
  'resource://gre/modules/Promise.jsm');


const SWIPE_MAX_DURATION = 200;


const MAX_MULTITOUCH = 125;

const MAX_CONSECUTIVE_GESTURE_DELAY = 200;

const DWELL_THRESHOLD = 250;

const SWIPE_MIN_DISTANCE = 0.4;

const TAP_MAX_RADIUS = 0.2;


const DIRECTNESS_COEFF = 1.44;

const IS_ANDROID = Utils.MozBuildApp === 'mobile/android' &&
  Utils.AndroidSdkVersion >= 14;


const ANDROID_TRIPLE_SWIPE_DELAY = 50;

const MOUSE_ID = 'mouse';

const EDGE = 0.1;

const TIMEOUT_MULTIPLIER = 1;








function Point(aPoint) {
  this.startX = this.x = aPoint.x;
  this.startY = this.y = aPoint.y;
  this.distanceTraveled = 0;
  this.totalDistanceTraveled = 0;
}

Point.prototype = {
  



  update: function Point_update(aPoint) {
    let lastX = this.x;
    let lastY = this.y;
    this.x = aPoint.x;
    this.y = aPoint.y;
    this.distanceTraveled = this.getDistanceToCoord(lastX, lastY);
    this.totalDistanceTraveled += this.distanceTraveled;
  },

  reset: function Point_reset() {
    this.distanceTraveled = 0;
    this.totalDistanceTraveled = 0;
  },

  






  getDistanceToCoord: function Point_getDistanceToCoord(aX, aY) {
    return Math.hypot(this.x - aX, this.y - aY);
  },

  


  get directDistanceTraveled() {
    return this.getDistanceToCoord(this.startX, this.startY);
  }
};





this.GestureSettings = { 
  



  swipeMaxDuration: SWIPE_MAX_DURATION * TIMEOUT_MULTIPLIER,

  



  maxMultitouch: MAX_MULTITOUCH * TIMEOUT_MULTIPLIER,

  



  maxConsecutiveGestureDelay:
    MAX_CONSECUTIVE_GESTURE_DELAY * TIMEOUT_MULTIPLIER,

  



  dwellThreshold: DWELL_THRESHOLD * TIMEOUT_MULTIPLIER,

  




  travelThreshold: 0.025
};






this.GestureTracker = { 
  



  reset: function GestureTracker_reset() {
    if (this.current) {
      this.current.clearTimer();
    }
    delete this.current;
  },

  






  _init: function GestureTracker__init(aDetail, aTimeStamp, aGesture = Tap) {
    
    if (aDetail.type !== 'pointerdown') {
      return;
    }
    let points = aDetail.points;
    let GestureConstructor = aGesture;
    if (IS_ANDROID && GestureConstructor === Tap && points.length === 1 &&
      points[0].identifier !== MOUSE_ID) {
      
      
      GestureConstructor = AndroidTap;
    }
    this._create(GestureConstructor);
    this._update(aDetail, aTimeStamp);
  },

  





  handle: function GestureTracker_handle(aDetail, aTimeStamp) {
    Logger.gesture(() => {
      return ['Pointer event', aDetail.type, 'at:', aTimeStamp,
        JSON.stringify(aDetail.points)];
    });
    this[this.current ? '_update' : '_init'](aDetail, aTimeStamp);
  },

  







  _create: function GestureTracker__create(aGesture, aTimeStamp, aPoints, aLastEvent) {
    this.current = new aGesture(aTimeStamp, aPoints, aLastEvent);  
    this.current.then(this._onFulfill.bind(this));
  },

  




  _update: function GestureTracker_update(aDetail, aTimeStamp) {
    this.current[aDetail.type](aDetail.points, aTimeStamp);
  },

  




  _onFulfill: function GestureTracker__onFulfill(aResult) {
    let {id, gestureType} = aResult;
    let current = this.current;
    
    
    if (!current || current.id !== id) {
      return;
    }
    
    if (gestureType) {
      this._create(gestureType, current.startTime, current.points,
        current.lastEvent);
    } else {
      delete this.current;
    }
  }
};











function compileDetail(aType, aPoints, keyMap = {x: 'startX', y: 'startY'}) {
  let touches = [];
  let maxDeltaX = 0;
  let maxDeltaY = 0;
  for (let identifier in aPoints) {
    let point = aPoints[identifier];
    let touch = {};
    for (let key in keyMap) {
      touch[key] = point[keyMap[key]];
    }
    touches.push(touch);
    let deltaX = point.x - point.startX;
    let deltaY = point.y - point.startY;
    
    if (Math.abs(maxDeltaX) < Math.abs(deltaX)) {
      maxDeltaX = deltaX;
    }
    if (Math.abs(maxDeltaY) < Math.abs(deltaY)) {
      maxDeltaY = deltaY;
    }
    
    
    point.reset();
  }
  return {
    type: aType,
    touches: touches,
    deltaX: maxDeltaX,
    deltaY: maxDeltaY
  };
}









function Gesture(aTimeStamp, aPoints = {}, aLastEvent = undefined) {
  this.startTime = Date.now();
  Logger.gesture('Creating', this.id, 'gesture.');
  this.points = aPoints;
  this.lastEvent = aLastEvent;
  this._deferred = Promise.defer();
  
  
  this.promise = this._deferred.promise.then(this._handleResolve.bind(this),
    this._handleReject.bind(this));
  this.startTimer(aTimeStamp);
}

Gesture.prototype = {
  



  _getDelay: function Gesture__getDelay() {
    
    
    
    
    return GestureSettings.maxConsecutiveGestureDelay;
  },

  


  clearTimer: function Gesture_clearTimer() {
    clearTimeout(this._timer);
    delete this._timer;
  },

  




  startTimer: function Gesture_startTimer(aTimeStamp) {
    this.clearTimer();
    let delay = this._getDelay(aTimeStamp);
    let handler = () => {
      delete this._timer;
      if (!this._inProgress) {
        this._deferred.reject();
      } else if (this._rejectToOnWait) {
        this._deferred.reject(this._rejectToOnWait);
      }
    };
    if (delay <= 0) {
      handler();
    } else {
      this._timer = setTimeout(handler, delay);
    }
  },

  



  then: function Gesture_then(aCallback) {
    this.promise.then(aCallback);
  },

  













  _update: function Gesture__update(aPoints, aType, aCanCreate = false, aNeedComplete = false) {
    let complete;
    let lastEvent;
    for (let point of aPoints) {
      let identifier = point.identifier;
      let gesturePoint = this.points[identifier];
      if (gesturePoint) {
        gesturePoint.update(point);
        if (aNeedComplete) {
          
          
          complete = true;
        }
        lastEvent = lastEvent || aType;
      } else if (aCanCreate) {
        
        this.points[identifier] =
          new Point(point);
        lastEvent = lastEvent || aType;
      }
    }
    this.lastEvent = lastEvent || this.lastEvent;
    
    if (this.test) {
      this.test(complete);
    }
    return complete;
  },

  



  _emit: function Gesture__emit(aDetail) {
    let evt = new Utils.win.CustomEvent('mozAccessFuGesture', {
      bubbles: true,
      cancelable: true,
      detail: aDetail
    });
    Utils.win.dispatchEvent(evt);
  },

  




  pointerdown: function Gesture_pointerdown(aPoints, aTimeStamp) {
    this._inProgress = true;
    this._update(aPoints, 'pointerdown',
      aTimeStamp - this.startTime < GestureSettings.maxMultitouch);
  },

  



  pointermove: function Gesture_pointermove(aPoints) {
    this._update(aPoints, 'pointermove');
  },

  



  pointerup: function Gesture_pointerup(aPoints) {
    let complete = this._update(aPoints, 'pointerup', false, true);
    if (complete) {
      this._deferred.resolve();
    }
  },

  




  resolveTo: null,

  


  get id() {
    delete this._id;
    this._id = this.type + this.startTime;
    return this._id;
  },

  







  _handleResolve: function Gesture__handleResolve() {
    if (this.isComplete) {
      return;
    }
    Logger.gesture('Resolving', this.id, 'gesture.');
    this.isComplete = true;
    let detail = this.compile();
    if (detail) {
      this._emit(detail);
    }
    return {
      id: this.id,
      gestureType: this.resolveTo
    };
  },

  







  _handleReject: function Gesture__handleReject(aRejectTo) {
    if (this.isComplete) {
      return;
    }
    Logger.gesture('Rejecting', this.id, 'gesture.');
    this.isComplete = true;
    return {
      id: this.id,
      gestureType: aRejectTo
    };
  },

  





  compile: function Gesture_compile() {
    return compileDetail(this.type, this.points);
  }
};




function ExploreGesture() {
  this.compile = () => {
    
    
    return compileDetail(this.type, this.points, {x: 'x', y: 'y'});
  };
}




function checkProgressGesture(aGesture) {
  aGesture._inProgress = true;
  if (aGesture.lastEvent === 'pointerup') {
    if (aGesture.test) {
      aGesture.test(true);
    }
    aGesture._deferred.resolve();
  }
}















function TravelGesture(aTimeStamp, aPoints, aLastEvent, aTravelTo = Explore, aThreshold = GestureSettings.travelThreshold) {
  Gesture.call(this, aTimeStamp, aPoints, aLastEvent);
  this._travelTo = aTravelTo;
  this._threshold = aThreshold;
}

TravelGesture.prototype = Object.create(Gesture.prototype);





TravelGesture.prototype.test = function TravelGesture_test() {
  for (let identifier in this.points) {
    let point = this.points[identifier];
    if (point.totalDistanceTraveled / Utils.dpi > this._threshold) {
      this._deferred.reject(this._travelTo);
      return;
    }
  }
};








function DwellEnd(aTimeStamp, aPoints, aLastEvent) {
  this._inProgress = true;
  
  TravelGesture.call(this, aTimeStamp, aPoints, aLastEvent);
  checkProgressGesture(this);
}

DwellEnd.prototype = Object.create(TravelGesture.prototype);
DwellEnd.prototype.type = 'dwellend';









function TapHoldEnd(aTimeStamp, aPoints, aLastEvent) {
  this._inProgress = true;
  
  TravelGesture.call(this, aTimeStamp, aPoints, aLastEvent);
  checkProgressGesture(this);
}

TapHoldEnd.prototype = Object.create(TravelGesture.prototype);
TapHoldEnd.prototype.type = 'tapholdend';










function DoubleTapHoldEnd(aTimeStamp, aPoints, aLastEvent) {
  this._inProgress = true;
  
  TravelGesture.call(this, aTimeStamp, aPoints, aLastEvent);
  checkProgressGesture(this);
}

DoubleTapHoldEnd.prototype = Object.create(TravelGesture.prototype);
DoubleTapHoldEnd.prototype.type = 'doubletapholdend';













function TapGesture(aTimeStamp, aPoints, aLastEvent, aRejectTo, aTravelTo) {
  this._rejectToOnWait = aRejectTo;
  
  TravelGesture.call(this, aTimeStamp, aPoints, aLastEvent, aTravelTo,
    TAP_MAX_RADIUS);
}

TapGesture.prototype = Object.create(TravelGesture.prototype);
TapGesture.prototype._getDelay = function TapGesture__getDelay() {
  
  
  
  return GestureSettings.dwellThreshold;
};








function Tap(aTimeStamp, aPoints, aLastEvent) {
  
  TapGesture.call(this, aTimeStamp, aPoints, aLastEvent, Dwell, Swipe);
}

Tap.prototype = Object.create(TapGesture.prototype);
Tap.prototype.type = 'tap';
Tap.prototype.resolveTo = DoubleTap;








function AndroidTap(aTimeStamp, aPoints, aLastEvent) {
  
  
  TapGesture.call(this, aTimeStamp, aPoints, aLastEvent, TapHold, Swipe);
}
AndroidTap.prototype = Object.create(TapGesture.prototype);

AndroidTap.prototype.type = 'doubletap';
AndroidTap.prototype.resolveTo = TripleTap;




AndroidTap.prototype.clearThreeFingerSwipeTimer = function AndroidTap_clearThreeFingerSwipeTimer() {
  clearTimeout(this._threeFingerSwipeTimer);
  delete this._threeFingerSwipeTimer;
};

AndroidTap.prototype.pointerdown = function AndroidTap_pointerdown(aPoints, aTimeStamp) {
  this.clearThreeFingerSwipeTimer();
  TapGesture.prototype.pointerdown.call(this, aPoints, aTimeStamp);
};

AndroidTap.prototype.pointermove = function AndroidTap_pointermove(aPoints) {
  this.clearThreeFingerSwipeTimer();
  this._moved = true;
  TapGesture.prototype.pointermove.call(this, aPoints);
};

AndroidTap.prototype.pointerup = function AndroidTap_pointerup(aPoints) {
  if (this._moved) {
    
    TapGesture.prototype.pointerup.call(this, aPoints);
  } else {
    
    
    this._threeFingerSwipeTimer = setTimeout(() => {
      delete this._threeFingerSwipeTimer;
      TapGesture.prototype.pointerup.call(this, aPoints);
    }, ANDROID_TRIPLE_SWIPE_DELAY);
  }
};









AndroidTap.prototype._handleReject = function AndroidTap__handleReject(aRejectTo) {
  let keys = Object.keys(this.points);
  if (aRejectTo === Swipe && keys.length === 1) {
    let key = keys[0];
    let point = this.points[key];
    
    this.points[key + '-copy'] = point;
  }
  return TapGesture.prototype._handleReject.call(this, aRejectTo);
};








function DoubleTap(aTimeStamp, aPoints, aLastEvent) {
  TapGesture.call(this, aTimeStamp, aPoints, aLastEvent, TapHold);
}

DoubleTap.prototype = Object.create(TapGesture.prototype);
DoubleTap.prototype.type = 'doubletap';
DoubleTap.prototype.resolveTo = TripleTap;








function TripleTap(aTimeStamp, aPoints, aLastEvent) {
  TapGesture.call(this, aTimeStamp, aPoints, aLastEvent, DoubleTapHold);
}

TripleTap.prototype = Object.create(TapGesture.prototype);
TripleTap.prototype.type = 'tripletap';








function ResolvedGesture(aTimeStamp, aPoints, aLastEvent) {
  Gesture.call(this, aTimeStamp, aPoints, aLastEvent);
  
  this._deferred.resolve();
}

ResolvedGesture.prototype = Object.create(Gesture.prototype);








function Dwell(aTimeStamp, aPoints, aLastEvent) {
  ResolvedGesture.call(this, aTimeStamp, aPoints, aLastEvent);
}

Dwell.prototype = Object.create(ResolvedGesture.prototype);
Dwell.prototype.type = 'dwell';
Dwell.prototype.resolveTo = DwellEnd;








function TapHold(aTimeStamp, aPoints, aLastEvent) {
  ResolvedGesture.call(this, aTimeStamp, aPoints, aLastEvent);
}

TapHold.prototype = Object.create(ResolvedGesture.prototype);
TapHold.prototype.type = 'taphold';
TapHold.prototype.resolveTo = TapHoldEnd;








function DoubleTapHold(aTimeStamp, aPoints, aLastEvent) {
  ResolvedGesture.call(this, aTimeStamp, aPoints, aLastEvent);
}

DoubleTapHold.prototype = Object.create(ResolvedGesture.prototype);
DoubleTapHold.prototype.type = 'doubletaphold';
DoubleTapHold.prototype.resolveTo = DoubleTapHoldEnd;








function Explore(aTimeStamp, aPoints, aLastEvent) {
  ExploreGesture.call(this);
  ResolvedGesture.call(this, aTimeStamp, aPoints, aLastEvent);
}

Explore.prototype = Object.create(ResolvedGesture.prototype);
Explore.prototype.type = 'explore';
Explore.prototype.resolveTo = ExploreEnd;








function ExploreEnd(aTimeStamp, aPoints, aLastEvent) {
  this._inProgress = true;
  ExploreGesture.call(this);
  
  TravelGesture.call(this, aTimeStamp, aPoints, aLastEvent);
  checkProgressGesture(this);
}

ExploreEnd.prototype = Object.create(TravelGesture.prototype);
ExploreEnd.prototype.type = 'exploreend';








function Swipe(aTimeStamp, aPoints, aLastEvent) {
  this._inProgress = true;
  this._rejectToOnWait = Explore;
  Gesture.call(this, aTimeStamp, aPoints, aLastEvent);
  checkProgressGesture(this);
}

Swipe.prototype = Object.create(Gesture.prototype);
Swipe.prototype.type = 'swipe';
Swipe.prototype._getDelay = function Swipe__getDelay(aTimeStamp) {
  
  
  return GestureSettings.swipeMaxDuration - this.startTime + aTimeStamp;
};






Swipe.prototype.test = function Swipe_test(aComplete) {
  if (!aComplete) {
    
    return;
  }
  let reject = true;
  
  
  for (let identifier in this.points) {
    let point = this.points[identifier];
    let directDistance = point.directDistanceTraveled;
    if (directDistance / Utils.dpi >= SWIPE_MIN_DISTANCE ||
      directDistance * DIRECTNESS_COEFF >= point.totalDistanceTraveled) {
      reject = false;
    }
  }
  if (reject) {
    this._deferred.reject(Explore);
  }
};





Swipe.prototype.compile = function Swipe_compile() {
  let type = this.type;
  let detail = compileDetail(type, this.points,
    {x1: 'startX', y1: 'startY', x2: 'x', y2: 'y'});
  let deltaX = detail.deltaX;
  let deltaY = detail.deltaY;
  let edge = EDGE * Utils.dpi;
  if (Math.abs(deltaX) > Math.abs(deltaY)) {
    
    let startPoints = [touch.x1 for (touch of detail.touches)];
    if (deltaX > 0) {
      detail.type = type + 'right';
      detail.edge = Math.min.apply(null, startPoints) <= edge;
    } else {
      detail.type = type + 'left';
      detail.edge =
        Utils.win.screen.width - Math.max.apply(null, startPoints) <= edge;
    }
  } else {
    
    let startPoints = [touch.y1 for (touch of detail.touches)];
    if (deltaY > 0) {
      detail.type = type + 'down';
      detail.edge = Math.min.apply(null, startPoints) <= edge;
    } else {
      detail.type = type + 'up';
      detail.edge =
        Utils.win.screen.height - Math.max.apply(null, startPoints) <= edge;
    }
  }
  return detail;
};
