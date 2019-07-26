


"use strict";

this.EXPORTED_SYMBOLS = ["CrossSlide"];


let CrossSlideThresholds = {
   SELECTIONSTART: 25,
   SPEEDBUMPSTART: 30,
   SPEEDBUMPEND: 50,
   REARRANGESTART: 80
};


let CrossSlidingState = {
  STARTED:  0,
  DRAGGING: 1,
  SELECTING: 2,
  SELECT_SPEED_BUMPING: 3,
  SPEED_BUMPING: 4,
  REARRANGING: 5,
  COMPLETED: 6
};

let CrossSlidingStateNames = [
  'started',
  'dragging',
  'selecting',
  'selectSpeedBumping',
  'speedBumping',
  'rearranging',
  'completed'
];





function isSelectable(aElement) {
  
  return aElement.nodeName == 'richgriditem';
}

function withinCone(aLen, aHeight) {
  
  return aLen > aHeight;
}

function getScrollAxisFromElement(aElement) {
  let elem = aElement,
      win = elem.ownerDocument.defaultView;
  let scrollX, scrollY;
  for (; elem && 1==elem.nodeType; elem = elem.parentNode) {
    let cs = win.getComputedStyle(elem);
    scrollX = (cs.overflowX=='scroll' || cs.overflowX=='auto');
    scrollY = (cs.overflowX=='scroll' || cs.overflowX=='auto');
    if (scrollX || scrollY) {
      break;
    }
  }
  return scrollX ? 'x' : 'y';
}

function pointFromTouchEvent(aEvent) {
  let touch = aEvent.touches[0];
  return { x: touch.clientX, y: touch.clientY };
}





function damp(aX) {
  return 2 / (1 + Math.exp(-2 * aX)) - 1;
}
function speedbump(aDelta, aStart, aEnd) {
  let x = Math.abs(aDelta);
  if (x <= aStart)
    return aDelta;
  let sign = aDelta / x;

  let d = aEnd - aStart;
  let damped = damp((x - aStart) / d);
  return sign * (aStart + (damped * d));
}


this.CrossSlide = {
  
  
  Thresholds: CrossSlideThresholds,
  State: CrossSlidingState,
  StateNames: CrossSlidingStateNames,
  
  speedbump: speedbump
};

function CrossSlideHandler(aNode, aThresholds) {
  this.node = aNode;
  this.thresholds = Object.create(CrossSlideThresholds);
  
  if (aThresholds) {
    for(let key in aThresholds)
      this.thresholds[key] = aThresholds[key];
  }
}

CrossSlideHandler.prototype = {
  node: null,
  drag: null,

  getCrossSlideState: function(aCrossAxisDistance, aScrollAxisDistance) {
    if (aCrossAxisDistance <= 0) {
      return CrossSlidingState.STARTED;
    }
    if (aCrossAxisDistance < this.thresholds.SELECTIONSTART) {
      return CrossSlidingState.DRAGGING;
    }
    if (aCrossAxisDistance < this.thresholds.SPEEDBUMPSTART) {
      return CrossSlidingState.SELECTING;
    }
    if (aCrossAxisDistance < this.thresholds.SPEEDBUMPEND) {
      return CrossSlidingState.SELECT_SPEED_BUMPING;
    }
    if (aCrossAxisDistance < this.thresholds.REARRANGESTART) {
      return CrossSlidingState.SPEED_BUMPING;
    }
    
    return -1;
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "touchstart":
        this._onTouchStart(aEvent);
        break;
      case "touchmove":
        this._onTouchMove(aEvent);
        break;
      case "touchend":
        this._onTouchEnd(aEvent);
        break;
    }
  },

  cancel: function(aEvent){
    this._fireProgressEvent("cancelled", aEvent);
    this.drag = null;
  },

  _onTouchStart: function onTouchStart(aEvent){
    if (aEvent.touches.length > 1)
      return;
    let touch = aEvent.touches[0];
     
     
    let target = aEvent.target;

    if (!isSelectable(target))
        return;

    let scrollAxis = getScrollAxisFromElement(target);

    this.drag = {
      scrollAxis: scrollAxis,
      crossAxis: (scrollAxis=='x') ? 'y' : 'x',
      origin: pointFromTouchEvent(aEvent),
      state: -1
    };
  },

  _onTouchMove: function(aEvent){
    if (!this.drag) {
      return;
    }

    aEvent.stopPropagation();

    if (aEvent.touches.length!==1) {
      
      return this.cancel(aEvent);
    }

    let startPt = this.drag.origin;
    let endPt = this.drag.position = pointFromTouchEvent(aEvent);

    let scrollAxis = this.drag.scrollAxis,
        crossAxis = this.drag.crossAxis;

    
    let crossAxisDistance = Math.abs(endPt[crossAxis] - startPt[crossAxis]);
    
    let scrollAxisDistance = Math.abs(endPt[scrollAxis] - startPt[scrollAxis]);

    let currState = this.drag.state;
    let newState = this.getCrossSlideState(crossAxisDistance, scrollAxisDistance);

    if (-1 == newState) {
      
      return this.cancel(aEvent);
    }

    let isWithinCone = withinCone(crossAxisDistance, scrollAxisDistance);

    if (currState < CrossSlidingState.SELECTING && !isWithinCone) {
      
      return;
    }
    if (currState >= CrossSlidingState.SELECTING && !isWithinCone) {
      
      
      return this.cancel(aEvent);
    }

    if (currState > newState) {
      
      return;
    }

    this.drag.state = newState;
    this._fireProgressEvent( CrossSlidingStateNames[newState], aEvent );
  },
  _onTouchEnd: function(aEvent){
    if (!this.drag)
      return;

    if (this.drag.state < CrossSlidingState.SELECTING) {
      return this.cancel(aEvent);
    }

    this._fireProgressEvent("completed", aEvent);
    this._fireSelectEvent(aEvent);
    this.drag = null;
  },

  




  _fireProgressEvent: function CrossSliding_fireEvent(aState, aEvent) {
    if (!this.drag)
        return;
    let event = this.node.ownerDocument.createEvent("Events");
    let crossAxis = this.drag.crossAxis;
    event.initEvent("MozCrossSliding", true, true);
    event.crossSlidingState = aState;
    event.position = this.drag.position;
    event.direction = this.drag.crossAxis;
    event.delta = this.drag.position[crossAxis] - this.drag.origin[crossAxis];
    aEvent.target.dispatchEvent(event);
  },

  



  _fireSelectEvent: function SelectTarget_fireEvent(aEvent) {
    let event = this.node.ownerDocument.createEvent("Events");
    event.initEvent("MozCrossSlideSelect", true, true);
    event.position = this.drag.position;
    aEvent.target.dispatchEvent(event);
  }
};
this.CrossSlide.Handler = CrossSlideHandler;
