






































var gWsDoLog = false;
var gWsLogDiv = null;

function logbase() {
  if (!gWsDoLog)
    return;

  if (gWsLogDiv == null && "console" in window) {
    console.log.apply(console, arguments);
  } else {
    var s = "";
    for (var i = 0; i < arguments.length; i++) {
      s += arguments[i] + " ";
    }
    s += "\n";
    if (gWsLogDiv) {
      gWsLogDiv.appendChild(document.createElementNS("http://www.w3.org/1999/xhtml", "br"));
      gWsLogDiv.appendChild(document.createTextNode(s));
    }

    dump(s);
  }
}

function dumpJSStack(stopAtNamedFunction) {
  let caller = Components.stack.caller;
  dump("\tStack: " + caller.name);
  while ((caller = caller.caller)) {
    dump(" <- " + caller.name);
    if (stopAtNamedFunction && caller.name != "anonymous")
      break;
  }
  dump("\n");
}

function log() {
  return;
  logbase.apply(window, arguments);
}

function log2() {
  return;
  logbase.apply(window, arguments);
}

let reportError = log;






function wsBorder(t, l, b, r) {
  this.setBorder(t, l, b, r);
}

wsBorder.prototype = {

  setBorder: function (t, l, b, r) {
    this.top = t;
    this.left = l;
    this.bottom = b;
    this.right = r;
  },

  toString: function () {
    return "[l:" + this.left + ",t:" + this.top + ",r:" + this.right + ",b:" + this.bottom + "]";
  }
};






function wsRect(x, y, w, h) {
  this.left = x;
  this.top = y;
  this.right = x+w;
  this.bottom = y+h;
}

wsRect.prototype = {

  get x() { return this.left; },
  get y() { return this.top; },
  get width() { return this.right - this.left; },
  get height() { return this.bottom - this.top; },
  set x(v) {
    let diff = this.left - v;
    this.left = v;
    this.right -= diff;
  },
  set y(v) {
    let diff = this.top - v;
    this.top = v;
    this.bottom -= diff;
  },
  set width(v) { this.right = this.left + v; },
  set height(v) { this.bottom = this.top + v; },

  setRect: function(x, y, w, h) {
    this.left = x;
    this.top = y;
    this.right = x+w;
    this.bottom = y+h;

    return this;
  },

  setBounds: function(t, l, b, r) {
    this.top = t;
    this.left = l;
    this.bottom = b;
    this.right = r;

    return this;
  },

  equals: function equals(r) {
    return (r != null       &&
            this.top == r.top &&
            this.left == r.left &&
            this.bottom == r.bottom &&
            this.right == r.right);
  },

  clone: function clone() {
    return new wsRect(this.left, this.top, this.right - this.left, this.bottom - this.top);
  },

  center: function center() {
    return [this.left + (this.right - this.left) / 2,
            this.top + (this.bottom - this.top) / 2];
  },

  centerRounded: function centerRounded() {
    return this.center().map(Math.round);
  },

  copyFrom: function(r) {
    this.top = r.top;
    this.left = r.left;
    this.bottom = r.bottom;
    this.right = r.right;

    return this;
  },

  copyFromTLBR: function(r) {
    this.left = r.left;
    this.top = r.top;
    this.right = r.right;
    this.bottom = r.bottom;

    return this;
  },

  translate: function(x, y) {
    this.left += x;
    this.right += x;
    this.top += y;
    this.bottom += y;

    return this;
  },

  
  union: function(rect) {
    let l = Math.min(this.left, rect.left);
    let r = Math.max(this.right, rect.right);
    let t = Math.min(this.top, rect.top);
    let b = Math.max(this.bottom, rect.bottom);

    return new wsRect(l, t, r-l, b-t);
  },

  toString: function() {
    return "[" + this.x + "," + this.y + "," + this.width + "," + this.height + "]";
  },

  expandBy: function(b) {
    this.left += b.left;
    this.right += b.right;
    this.top += b.top;
    this.bottom += b.bottom;
    return this;
  },

  contains: function(other) {
    return !!(other.left >= this.left &&
              other.right <= this.right &&
              other.top >= this.top &&
              other.bottom <= this.bottom);
  },

  intersect: function(r2) {
    let xmost1 = this.right;
    let xmost2 = r2.right;

    let x = Math.max(this.left, r2.left);

    let temp = Math.min(xmost1, xmost2);
    if (temp <= x)
      return null;

    let width = temp - x;

    let ymost1 = this.bottom;
    let ymost2 = r2.bottom;
    let y = Math.max(this.top, r2.top);

    temp = Math.min(ymost1, ymost2);
    if (temp <= y)
      return null;

    let height = temp - y;

    return new wsRect(x, y, width, height);
  },

  intersects: function(other) {
    let xok = (other.left > this.left && other.left < this.right) ||
      (other.right > this.left && other.right < this.right) ||
      (other.left <= this.left && other.right >= this.right);
    let yok = (other.top > this.top && other.top < this.bottom) ||
      (other.bottom > this.top && other.bottom < this.bottom) ||
      (other.top <= this.top && other.bottom >= this.bottom);
    return xok && yok;
  },

  




  restrictTo: function restrictTo(r2) {
    let xmost1 = this.right;
    let xmost2 = r2.right;

    let x = Math.max(this.left, r2.left);

    let temp = Math.min(xmost1, xmost2);
    if (temp <= x)
      throw "Intersection is empty but rects cannot be empty";

    let width = temp - x;

    let ymost1 = this.bottom;
    let ymost2 = r2.bottom;
    let y = Math.max(this.top, r2.top);

    temp = Math.min(ymost1, ymost2);
    if (temp <= y)
      throw "Intersection is empty but rects cannot be empty";

    let height = temp - y;

    return this.setRect(x, y, width, height);
  },

  







  expandToContain: function extendTo(rect) {
    let l = Math.min(this.left, rect.left);
    let r = Math.max(this.right, rect.right);
    let t = Math.min(this.top, rect.top);
    let b = Math.max(this.bottom, rect.bottom);

    return this.setRect(l, t, r-l, b-t);
  },

  round: function round(scale) {
    if (!scale) scale = 1;

    this.left = Math.floor(this.left * scale) / scale;
    this.top = Math.floor(this.top * scale) / scale;
    this.right = Math.ceil(this.right * scale) / scale;
    this.bottom = Math.ceil(this.bottom * scale) / scale;

    return this;
  },

  scale: function scale(xscl, yscl) {
    this.left *= xscl;
    this.right *= xscl;
    this.top *= yscl;
    this.bottom *= yscl;

    return this;
  }
};




















function WidgetStack(el, ew, eh) {
  this.init(el, ew, eh);
}

WidgetStack.prototype = {
  
  _el: null,

  
  _widgetState: null,

  
  _barriers: null,

  
  
  _viewport: null,

  
  _viewportBounds: null,
  
  
  _viewportOverflow: null,

  
  _pannableBounds: null,
  get pannableBounds() {
    if (!this._pannableBounds) {
      this._pannableBounds = this._viewportBounds.clone()
                                 .expandBy(this._viewportOverflow);
    }
    return this._pannableBounds.clone();
  },

  
  _viewingRect: null,

  
  
  
  globalOffsetX: 0,
  globalOffsetY: 0,

  
  _constrainToViewport: true,

  _viewportUpdateInterval: -1,
  _viewportUpdateTimeout: -1,

  _viewportUpdateHandler: null,
  _panHandler: null,

  _dragState: null,

  _skipViewportUpdates: 0,
  _forceViewportUpdate: false,

  
  
  
  
  init: function (el, ew, eh) {
    this._el = el;
    this._widgetState = {};
    this._barriers = [];

    let rect = this._el.getBoundingClientRect();
    let width = rect.width;
    let height = rect.height;

    if (ew != undefined && eh != undefined) {
      width = ew;
      height = eh;
    }

    this._viewportOverflow = new wsBorder(0, 0, 0, 0);

    this._viewingRect = new wsRect(0, 0, width, height);

    
    let children = this._el.childNodes;
    for (let i = 0; i < children.length; i++) {
      let c = this._el.childNodes[i];
      if (c.tagName == "spacer")
        this._addNewBarrierFromSpacer(c);
      else
        this._addNewWidget(c);
    }

    
    this._updateWidgets();

    if (this._viewport) {
      this._viewportBounds = new wsRect(0, 0, this._viewport.rect.width, this._viewport.rect.height);
    } else {
      this._viewportBounds = new wsRect(0, 0, 0, 0);
    }
  },

  
  
  
  
  moveWidgetBy: function (wid, x, y) {
    let state = this._getState(wid);

    state.rect.x += x;
    state.rect.y += y;

    this._commitState(state);
  },

  
  
  
  
  
  
  panBy: function panBy(dx, dy, ignoreBarriers) {
    dx = Math.round(dx);
    dy = Math.round(dy);

    if (dx == 0 && dy == 0)
      return false;

    let needsDragWrap = !this._dragging;

    if (needsDragWrap)
      this.dragStart(0, 0);

    let panned = this._panBy(dx, dy, ignoreBarriers);

    if (needsDragWrap)
      this.dragStop();

    return panned;
  },

  
  
  
  panTo: function panTo(x, y) {
    if (x == undefined || x == null)
      x = this._viewingRect.x;
    if (y == undefined || y == null)
      y = this._viewingRect.y;
    this.panBy(x - this._viewingRect.x, y - this._viewingRect.y, true);
  },

  
  
  
  
  freeze: function (wid) {
    let state = this._getState(wid);

    state.frozen = true;
  },

  unfreeze: function (wid) {
    let state = this._getState(wid);
    if (!state.frozen)
      return;

    state.frozen = false;
    this._commitState(state);
  },

  
  
  moveFrozenTo: function (wid, x, y) {
    let state = this._getState(wid);
    if (!state.frozen)
      throw "moveFrozenTo on non-frozen widget " + wid;

    state.widget.setAttribute("left", x);
    state.widget.setAttribute("top", y);
  },

  
  
  
  
  moveUnfrozenTo: function (wid, x, y) {
    delete this._widgetState[wid];
    let widget = document.getElementById(wid);
    if (x) widget.setAttribute("left", x);
    if (y) widget.setAttribute("top", y);
    this._addNewWidget(widget);
    this._updateWidgets();
  },

  
  get viewportVisibleRect () {
    let rect = this._viewportBounds.intersect(this._viewingRect);
    if (!rect)
        rect = new wsRect(0, 0, 0, 0);
    return rect;
  },

  isWidgetFrozen: function isWidgetFrozen(wid) {
    return this._getState(wid).frozen;
  },

  
  
  isWidgetVisible: function (wid) {
    let state = this._getState(wid);
    let visibleStackRect = new wsRect(0, 0, this._viewingRect.width, this._viewingRect.height);

    return visibleStackRect.intersects(state.rect);
  },

  
  getWidgetVisibility: function (wid) {
    let state = this._getState(wid);
    let visibleStackRect = new wsRect(0, 0, this._viewingRect.width, this._viewingRect.height);

    let visibleRect = visibleStackRect.intersect(state.rect);
    if (visibleRect)
      return [visibleRect.width / state.rect.width, visibleRect.height / state.rect.height]

    return [0, 0];
  },

  
  offsetAll: function (x, y) {
    this.globalOffsetX += x;
    this.globalOffsetY += y;

    for each (let state in this._widgetState) {
      state.rect.x += x;
      state.rect.y += y;

      this._commitState(state);
    }
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  setViewportBounds: function setViewportBounds() {
    let oldBounds = this._viewportBounds.clone();

    if (arguments.length == 1) {
      this._viewportBounds.copyFromTLBR(arguments[0]);
    } else if (arguments.length == 2) {
      this._viewportBounds.setRect(0, 0, arguments[0], arguments[1]);
    } else if (arguments.length == 4) {
      this._viewportBounds.setBounds(arguments[0],
      arguments[1],
      arguments[2],
      arguments[3]);
    } else {
      throw "Invalid number of arguments to setViewportBounds";
    }

    let vp = this._viewport;

    let dleft = this._viewportBounds.left - oldBounds.left;
    let dright = this._viewportBounds.right - oldBounds.right;
    let dtop = this._viewportBounds.top - oldBounds.top;
    let dbottom = this._viewportBounds.bottom - oldBounds.bottom;

    

    
    for each (let state in this._widgetState) {
      if (state.vpRelative) {
        
        if (state.vpOffsetXBefore) {
          state.rect.x += dleft;
        } else {
          state.rect.x += dright;
        }

        if (state.vpOffsetYBefore) {
          state.rect.y += dtop;
        } else {
          state.rect.y += dbottom;
        }

        
        this._commitState(state);
      }
    }

    for (let bid in this._barriers) {
      let barrier = this._barriers[bid];

      

      if (barrier.vpRelative) {
        if (barrier.type == "vertical") {
          let q = "v barrier moving from " + barrier.x + " to ";
          if (barrier.vpOffsetXBefore) {
            barrier.x += dleft;
          } else {
            barrier.x += dright;
          }
          
        } else if (barrier.type == "horizontal") {
          let q = "h barrier moving from " + barrier.y + " to ";
          if (barrier.vpOffsetYBefore) {
            barrier.y += dtop;
          } else {
            barrier.y += dbottom;
          }
          
        }
      }
    }

    
    this._pannableBounds = null;

    
    this._adjustViewingRect();

    this._viewportUpdate(0, 0, true);
  },

  
  
  
  
  
  
  
  setViewportHandler: function (uh) {
    this._viewportUpdateHandler = uh;
  },

  
  
  
  
  
  setPanHandler: function (uh) {
    this._panHandler = uh;
  },

  
  dragStart: function dragStart(clientX, clientY) {
    log("(dragStart)", clientX, clientY);

    if (this._dragState) {
      reportError("dragStart with drag already in progress? what?");
      this._dragState = null;
    }

    this._dragState = { };

    let t = Date.now();

    this._dragState.barrierState = [];

    this._dragState.startTime = t;
    
    this._dragState.outerStartX = clientX;
    this._dragState.outerStartY = clientY;

    this._dragCoordsFromClient(clientX, clientY, t);

    this._dragState.outerLastUpdateDX = 0;
    this._dragState.outerLastUpdateDY = 0;

    if (this._viewport) {
      
      
      this._viewport.dragStartRect = this._viewport.rect.clone();
    }

    this._dragState.dragging = true;
  },

  _viewportDragUpdate: function viewportDragUpdate() {
    let vws = this._viewport;
    this._viewportUpdate((vws.dragStartRect.x - vws.rect.x),
                         (vws.dragStartRect.y - vws.rect.y));
  },

  
  dragStop: function dragStop() {
    log("(dragStop)");

    if (!this._dragging)
      return;

    if (this._viewportUpdateTimeout != -1)
      clearTimeout(this._viewportUpdateTimeout);

    this._viewportDragUpdate();

    this._dragState = null;
  },

  
  dragMove: function dragMove(clientX, clientY) {
    if (!this._dragging)
      return false;

    this._dragCoordsFromClient(clientX, clientY);

    let panned = this._dragUpdate();

    if (this._viewportUpdateInterval != -1) {
      if (this._viewportUpdateTimeout != -1)
        clearTimeout(this._viewportUpdateTimeout);
      let self = this;
      this._viewportUpdateTimeout = setTimeout(function () { self._viewportDragUpdate(); }, this._viewportUpdateInterval);
    }

    return panned;
  },

  
  dragBy: function dragBy(dx, dy) {
    return this.dragMove(this._dragState.outerCurX + dx, this._dragState.outerCurY + dy);
  },

  
  
  updateSize: function updateSize(width, height) {
    if (width == undefined || height == undefined) {
      let rect = this._el.getBoundingClientRect();
      width = rect.width;
      height = rect.height;
    }

    
    
    

    
    
    
    
    

    this._viewingRect.width = width;
    this._viewingRect.height = height;

    
    
    
    this.beginUpdateBatch();
    this._adjustViewingRect();
    this.endUpdateBatch();
  },

  beginUpdateBatch: function startUpdate() {
    if (!this._skipViewportUpdates) {
      this._startViewportBoundsString = this._viewportBounds.toString();
      this._forceViewportUpdate = false;
    }
    this._skipViewportUpdates++;
  },

  endUpdateBatch: function endUpdate(aForceRedraw) {
    if (!this._skipViewportUpdates)
      throw new Error("Unbalanced call to endUpdateBatch");

    this._forceViewportUpdate = this._forceViewportUpdate || aForceRedraw;

    this._skipViewportUpdates--;
    if (this._skipViewportUpdates)
      return;

    let boundsSizeChanged =
      this._startViewportBoundsString != this._viewportBounds.toString();
    this._callViewportUpdateHandler(boundsSizeChanged || this._forceViewportUpdate);
  },

  
  
  

  _updateWidgetRect: function(state) {
    
    
    if (state == this._viewport)
      return;

    let w = state.widget;
    let x = w.getAttribute("left") || 0;
    let y = w.getAttribute("top") || 0;
    let rect = w.getBoundingClientRect();
    state.rect = new wsRect(parseInt(x), parseInt(y),
                            rect.right - rect.left,
                            rect.bottom - rect.top);
    if (w.hasAttribute("widgetwidth") && w.hasAttribute("widgetheight")) {
      state.rect.width = parseInt(w.getAttribute("widgetwidth"));
      state.rect.height = parseInt(w.getAttribute("widgetheight"));
    }
  },

  _dumpRects: function () {
    dump("WidgetStack:\n");
    dump("\tthis._viewportBounds: " + this._viewportBounds + "\n");
    dump("\tthis._viewingRect: " + this._viewingRect + "\n");
    dump("\tthis._viewport.viewportInnerBounds: " + this._viewport.viewportInnerBounds + "\n");
    dump("\tthis._viewport.rect: " + this._viewport.rect + "\n");
    dump("\tthis._viewportOverflow: " + this._viewportOverflow + "\n");
    dump("\tthis.pannableBounds: " + this.pannableBounds + "\n");
  },

  
  
  _adjustViewingRect: function _adjustViewingRect() {
    let vr = this._viewingRect;
    let pb = this.pannableBounds;

    if (pb.contains(vr))
      return; 

    
    
    if (vr.height > pb.height || vr.width > pb.width)
      return;

    let panX = 0, panY = 0;
    if (vr.right > pb.right)
      panX = pb.right - vr.right;
    else if (vr.left < pb.left)
      panX = pb.left - vr.left;

    if (vr.bottom > pb.bottom)
      panY = pb.bottom - vr.bottom;
    else if(vr.top < pb.top)
      panY = pb.top - vr.top;

    this.panBy(panX, panY, true);
  },

  _getState: function (wid) {
    let w = this._widgetState[wid];
    if (!w)
      throw "Unknown widget id '" + wid + "'; widget not in stack";
    return w;
  },

  get _dragging() {
    return this._dragState && this._dragState.dragging;
  },

  _viewportUpdate: function _viewportUpdate(dX, dY, boundsChanged) {
    if (!this._viewport)
      return;

    this._viewportUpdateTimeout = -1;

    let vws = this._viewport;
    let vwib = vws.viewportInnerBounds;
    let vpb = this._viewportBounds;

    
    
    
    let [ignoreX, ignoreY] = this._offsets || [0, 0];
    let rx = dX - ignoreX;
    let ry = dY - ignoreY;

    [dX, dY] = this._rectTranslateConstrain(rx, ry, vwib, vpb);

    
    
    this._offsets = [dX - rx, dY - ry];

    
    vwib.translate(dX, dY);
    vws.rect.translate(dX, dY);
    this._commitState(vws);

    
    
    vws.dragStartRect = vws.rect.clone();

    this._callViewportUpdateHandler(boundsChanged);
  },

  _callViewportUpdateHandler: function _callViewportUpdateHandler(boundsChanged) {
    if (!this._viewport || !this._viewportUpdateHandler || this._skipViewportUpdates)
      return;

    let vwb = this._viewportBounds.clone();

    let vwib = this._viewport.viewportInnerBounds.clone();

    let vis = this.viewportVisibleRect;

    vwib.left += this._viewport.offsetLeft;
    vwib.top += this._viewport.offsetTop;
    vwib.right += this._viewport.offsetRight;
    vwib.bottom += this._viewport.offsetBottom;

    this._viewportUpdateHandler.apply(window, [vwb, vwib, vis, boundsChanged]);
  },

  _dragCoordsFromClient: function (cx, cy, t) {
    this._dragState.curTime = t ? t : Date.now();
    this._dragState.outerCurX = cx;
    this._dragState.outerCurY = cy;

    let dx = this._dragState.outerCurX - this._dragState.outerStartX;
    let dy = this._dragState.outerCurY - this._dragState.outerStartY;
    this._dragState.outerDX = dx;
    this._dragState.outerDY = dy;
  },

  _panHandleBarriers: function (dx, dy) {
    
    
    

    let vr = this._viewingRect;

    

    
    
    
    let barrier_y = null, barrier_x = null;
    let barrier_dy = 0, barrier_dx = 0;

    for (let i = 0; i < this._barriers.length; i++) {
      let b = this._barriers[i];

      

      if (dx != 0 && b.type == "vertical") {
        if (barrier_x != null) {
          delete this._dragState.barrierState[i];
          continue;
        }

        let alreadyKnownDistance = this._dragState.barrierState[i] || 0;

        

        let dbx = 0;

        

        if ((vr.left <= b.x && vr.left+dx > b.x) ||
            (vr.left >= b.x && vr.left+dx < b.x))
        {
          dbx = b.x - vr.left;
        } else if ((vr.right <= b.x && vr.right+dx > b.x) ||
                   (vr.right >= b.x && vr.right+dx < b.x))
        {
          dbx = b.x - vr.right;
        } else {
          delete this._dragState.barrierState[i];
          continue;
        }

        let leftoverDistance = dbx - dx;

        

        let dist = Math.abs(leftoverDistance + alreadyKnownDistance) - b.size;

        if (dist >= 0) {
          if (dx < 0)
            dbx -= dist;
          else
            dbx += dist;
          delete this._dragState.barrierState[i];
        } else {
          dbx = 0;
          this._dragState.barrierState[i] = leftoverDistance + alreadyKnownDistance;
        }

        

        if (Math.abs(barrier_dx) <= Math.abs(dbx)) {
          barrier_x = b;
          barrier_dx = dbx;

          
        }
      }

      if (dy != 0 && b.type == "horizontal") {
        if (barrier_y != null) {
          delete this._dragState.barrierState[i];
          continue;
        }

        let alreadyKnownDistance = this._dragState.barrierState[i] || 0;

        

        let dby = 0;

        

        if ((vr.top <= b.y && vr.top+dy > b.y) ||
            (vr.top >= b.y && vr.top+dy < b.y))
        {
          dby = b.y - vr.top;
        } else if ((vr.bottom <= b.y && vr.bottom+dy > b.y) ||
                   (vr.bottom >= b.y && vr.bottom+dy < b.y))
        {
          dby = b.y - vr.bottom;
        } else {
          delete this._dragState.barrierState[i];
          continue;
        }

        let leftoverDistance = dby - dy;

        

        let dist = Math.abs(leftoverDistance + alreadyKnownDistance) - b.size;

        if (dist >= 0) {
          if (dy < 0)
            dby -= dist;
          else
            dby += dist;
          delete this._dragState.barrierState[i];
        } else {
          dby = 0;
          this._dragState.barrierState[i] = leftoverDistance + alreadyKnownDistance;
        }

        

        if (Math.abs(barrier_dy) <= Math.abs(dby)) {
          barrier_y = b;
          barrier_dy = dby;

          
        }
      }
    }

    if (barrier_x) {
      
      dx = barrier_dx;
    }

    if (barrier_y) {
      dy = barrier_dy;
    }

    return [dx, dy];
  },

  _panBy: function _panBy(dx, dy, ignoreBarriers) {
    let vr = this._viewingRect;

    
    
    if (!ignoreBarriers)
      [dx, dy] = this._panHandleBarriers(dx, dy);

    
    
    
    
    
    [dx, dy] = this._rectTranslateConstrain(dx, dy, vr, this.pannableBounds);

    
    
    if (dx == 0 && dy == 0)
      return false;

    
    vr.x += dx;
    vr.y += dy;

    
    
    
    
    for each (let state in this._widgetState) {
      if (!state.ignoreX)
        state.rect.x -= dx;
      if (!state.ignoreY)
        state.rect.y -= dy;

      this._commitState(state);
    }

    



    if (!this._skipViewportUpdates && this._panHandler)
      this._panHandler.apply(window, [vr.clone(), dx, dy]);

    return true;
  },

  _dragUpdate: function _dragUpdate() {
    let dx = this._dragState.outerLastUpdateDX - this._dragState.outerDX;
    let dy = this._dragState.outerLastUpdateDY - this._dragState.outerDY;

    this._dragState.outerLastUpdateDX = this._dragState.outerDX;
    this._dragState.outerLastUpdateDY = this._dragState.outerDY;

    return this.panBy(dx, dy);
  },

  
  
  
  _addNewWidget: function (w) {
    let wid = w.getAttribute("id");
    if (!wid) {
      reportError("WidgetStack: child widget without id!");
      return;
    }

    if (w.getAttribute("hidden") == "true")
      return;

    let state = {
      widget: w,
      id: wid,

      viewport: false,
      ignoreX: false,
      ignoreY: false,
      sticky: false,
      frozen: false,
      vpRelative: false,

      offsetLeft: 0,
      offsetTop: 0,
      offsetRight: 0,
      offsetBottom: 0
    };

    this._updateWidgetRect(state);

    if (w.hasAttribute("constraint")) {
      let cs = w.getAttribute("constraint").split(",");
      for each (let s in cs) {
        if (s == "ignore-x")
          state.ignoreX = true;
        else if (s == "ignore-y")
          state.ignoreY = true;
        else if (s == "sticky")
          state.sticky = true;
        else if (s == "frozen") {
          state.frozen = true;
        } else if (s == "vp-relative")
          state.vpRelative = true;
      }
    }

    if (w.hasAttribute("viewport")) {
      if (this._viewport)
        reportError("WidgetStack: more than one viewport canvas in stack!");

      this._viewport = state;
      state.viewport = true;

      if (w.hasAttribute("vptargetx") && w.hasAttribute("vptargety") &&
          w.hasAttribute("vptargetw") && w.hasAttribute("vptargeth"))
      {
        let wx = parseInt(w.getAttribute("vptargetx"));
        let wy = parseInt(w.getAttribute("vptargety"));
        let ww = parseInt(w.getAttribute("vptargetw"));
        let wh = parseInt(w.getAttribute("vptargeth"));

        state.offsetLeft = state.rect.left - wx;
        state.offsetTop = state.rect.top - wy;
        state.offsetRight = state.rect.right - (wx + ww);
        state.offsetBottom = state.rect.bottom - (wy + wh);

        state.rect = new wsRect(wx, wy, ww, wh);
      }

      
      state.viewportInnerBounds = new wsRect(0, 0, state.rect.width, state.rect.height);
    }

    this._widgetState[wid] = state;

    log ("(New widget: " + wid + (state.viewport ? " [viewport]" : "") + " at: " + state.rect + ")");
  },

  _removeWidget: function (w) {
    let wid = w.getAttribute("id");
    delete this._widgetState[wid];
    this._updateWidgets();
  },

  
  
  
  
  
  
  
  _updateWidgets: function () {
    let vp = this._viewport;

    let ofRect = this._viewingRect.clone();

    for each (let state in this._widgetState) {
      if (vp && state.vpRelative) {
        
        if (state.rect.left >= vp.rect.right) {
          state.vpOffsetXBefore = false;
          state.vpOffsetX = state.rect.left - vp.rect.width;
        } else {
          state.vpOffsetXBefore = true;
          state.vpOffsetX = state.rect.left - vp.rect.left;
        }

        if (state.rect.top >= vp.rect.bottom) {
          state.vpOffsetYBefore = false;
          state.vpOffsetY = state.rect.top - vp.rect.height;
        } else {
          state.vpOffsetYBefore = true;
          state.vpOffsetY = state.rect.top - vp.rect.top;
        }

        log("widget", state.id, "offset", state.vpOffsetX, state.vpOffsetXBefore ? "b" : "a", state.vpOffsetY, state.vpOffsetYBefore ? "b" : "a", "rect", state.rect);
      }
    }

    this._updateViewportOverflow();
  },

  
  _updateViewportOverflow: function() {
    let vp = this._viewport;
    if (!vp)
      return;

    let ofRect = new wsRect(0, 0, this._viewingRect.width, this._viewingRect.height);

    for each (let state in this._widgetState) {
      if (vp && state.vpRelative) {
        ofRect.left = Math.min(ofRect.left, state.rect.left);
        ofRect.top = Math.min(ofRect.top, state.rect.top);
        ofRect.right = Math.max(ofRect.right, state.rect.right);
        ofRect.bottom = Math.max(ofRect.bottom, state.rect.bottom);
      }
    }

    
    
    
    this._viewportOverflow = new wsBorder(
       Math.round(Math.min(ofRect.top, 0)),
       Math.round(Math.min(ofRect.left, 0)),
       Math.round(Math.max(ofRect.bottom - vp.rect.height, 0)),
       Math.round(Math.max(ofRect.right - vp.rect.width, 0))
    );

    
    
    this._pannableBounds = null;
  },

  _widgetBounds: function () {
    let r = new wsRect(0,0,0,0);

    for each (let state in this._widgetState)
      r = r.union(state.rect);

    return r;
  },

  _commitState: function (state) {
    
    
    if (state.frozen)
      return;
    let w = state.widget;
    let l = state.rect.x + state.offsetLeft;
    let t = state.rect.y + state.offsetTop;

    
    if (state._left != l) {
      state._left = l;
      w.setAttribute("left", l);
    }

    if (state._top != t) {
      state._top = t;
      w.setAttribute("top", t);
    }
  },

  
  
  _rectTranslateConstrain: function (dx, dy, rect, bounds) {
    let newX, newY;

    
    let woverflow = rect.width > bounds.width;
    let hoverflow = rect.height > bounds.height;
    if (woverflow || hoverflow) {
      let intersection = rect.intersect(bounds);
      let newIntersection = rect.clone().translate(dx, dy).intersect(bounds);
      if (woverflow)
        newX = (newIntersection.width > intersection.width) ? rect.x + dx : rect.x;
      if (hoverflow)
        newY = (newIntersection.height > intersection.height) ? rect.y + dy : rect.y;
    }

    
    
    
    if (isNaN(newX))
      newX = Math.min(Math.max(bounds.left, rect.x + dx), bounds.right - rect.width);
    if (isNaN(newY))
      newY = Math.min(Math.max(bounds.top, rect.y + dy), bounds.bottom - rect.height);

    return [newX - rect.x, newY - rect.y];
  },

  
  _addNewBarrierFromSpacer: function (el) {
    let t = el.getAttribute("barriertype");

    
    
    

    if (t != "horizontal" &&
        t != "vertical")
    {
      throw "Invalid barrier type: " + t;
    }

    let x, y;

    let barrier = {};
    let vp = this._viewport;

    barrier.type = t;

    if (el.getAttribute("left"))
      barrier.x = parseInt(el.getAttribute("left"));
    else if (el.getAttribute("top"))
      barrier.y = parseInt(el.getAttribute("top"));
    else
      throw "Barrier without top or left attribute";

    if (el.getAttribute("size"))
      barrier.size = parseInt(el.getAttribute("size"));
    else
      barrier.size = 10;

    if (el.hasAttribute("constraint")) {
      let cs = el.getAttribute("constraint").split(",");
      for each (let s in cs) {
        if (s == "ignore-x")
          barrier.ignoreX = true;
        else if (s == "ignore-y")
          barrier.ignoreY = true;
        else if (s == "sticky")
          barrier.sticky = true;
        else if (s == "frozen") {
          barrier.frozen = true;
        } else if (s == "vp-relative")
          barrier.vpRelative = true;
      }
    }

    if (barrier.vpRelative) {
      if (barrier.type == "vertical") {
        if (barrier.x >= vp.rect.right) {
          barrier.vpOffsetXBefore = false;
          barrier.vpOffsetX = barrier.x - vp.rect.right;
        } else {
          barrier.vpOffsetXBefore = true;
          barrier.vpOffsetX = barrier.x - vp.rect.left;
        }
      } else if (barrier.type == "horizontal") {
        if (barrier.y >= vp.rect.bottom) {
          barrier.vpOffsetYBefore = false;
          barrier.vpOffsetY = barrier.y - vp.rect.bottom;
        } else {
          barrier.vpOffsetYBefore = true;
          barrier.vpOffsetY = barrier.y - vp.rect.top;
        }

        
      }
    }

    this._barriers.push(barrier);
  }
};
