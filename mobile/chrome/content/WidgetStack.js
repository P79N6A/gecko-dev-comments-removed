






































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
  _t: 0, _l: 0, _b: 0, _r: 0,

  get left() { return this._l; },
  get right() { return this._r; },
  get top() { return this._t; },
  get bottom() { return this._b; },

  set left(v) { this._l = v; },
  set right(v) { this._r = v; },
  set top(v) { this._t = v; },
  set bottom(v) { this._b = v; },

  setBorder: function (t, l, b, r) {
    this._t = t;
    this._l = l;
    this._b = b;
    this._r = r;
  },

  toString: function () {
    return "[l:" + this._l + ",t:" + this._t + ",r:" + this._r + ",b:" + this._b + "]";
  }
};






function wsRect(x, y, w, h) {
  this.setRect(x, y, w, h);
}

wsRect.prototype = {
  _l: 0, _t: 0, _b: 0, _r: 0,

  get x() { return this._l; },
  get y() { return this._t; },
  get width() { return this._r - this._l; },
  get height() { return this._b - this._t; },
  set x(v) { 
    let diff = this._l - v;
    this._l = v;
    this._r -= diff; 
  },
  set y(v) { 
    let diff = this._t - v;
    this._t = v;
    this._b -= diff;
  },
  set width(v) { this._r = this._l + v; },
  set height(v) { this._b = this._t + v; },

  get left() { return this._l; },
  get right() { return this._r; },
  get top() { return this._t; },
  get bottom() { return this._b; },

  set left(v) { this._l = v; },
  set right(v) { this._r = v; },
  set top(v) { this._t = v; },
  set bottom(v) { this._b = v; },

  setRect: function(x, y, w, h) {
    this._l = x;
    this._t = y;
    this._r = x+w;
    this._b = y+h;

    return this;
  },

  setBounds: function(t, l, b, r) {
    this._t = t;
    this._l = l;
    this._b = b;
    this._r = r;

    return this;
  },

  clone: function() {
    return new wsRect(this._l, this._t, this.width, this.height);
  },

  copyFrom: function(r) {
    this._t = r._t;
    this._l = r._l;
    this._b = r._b;
    this._r = r._r;

    return this;
  },

  copyFromTLBR: function(r) {
    this._l = r.left;
    this._t = r.top;
    this._r = r.right;
    this._b = r.bottom;

    return this;
  },

  translate: function(x, y) {
    this._l += x;
    this._r += x;
    this._t += y;
    this._b += y;

    return this;
  },

  
  union: function(rect) {
    let l = Math.min(this._l, rect._l);
    let r = Math.max(this._r, rect._r);
    let t = Math.min(this._t, rect._t);
    let b = Math.max(this._b, rect._b);

    return new wsRect(l, t, r-l, b-t);
  },

  toString: function() {
    return "[" + this.x + "," + this.y + "," + this.width + "," + this.height + "]";
  },

  expandBy: function(b) {
    this._l += b.left;
    this._r += b.right;
    this._t += b.top;
    this._b += b.bottom;
    return this;
  },

  contains: function(other) {
    return !!(other._l >= this._l &&
              other._r <= this._r &&
              other._t >= this._t &&
              other._b <= this._b);
  },

  intersect: function(r2) {
    let xmost1 = this._r;
    let xmost2 = r2._r;

    let x = Math.max(this._l, r2._l);
    
    let temp = Math.min(xmost1, xmost2);
    if (temp <= x)
      return null;

    let width = temp - x;
    
    let ymost1 = this._b;
    let ymost2 = r2._b;
    let y = Math.max(this._t, r2._t);

    temp = Math.min(ymost1, ymost2);
    if (temp <= y)
      return null;

    let height = temp - y;

    return new wsRect(x, y, width, height);
  },

  intersects: function(other) {
    let xok = (other._l > this._l && other._l < this._r) ||
      (other._r > this._l && other._r < this._r) ||
      (other._l <= this._l && other._r >= this._r);
    let yok = (other._t > this._t && other._t < this._b) ||
      (other._b > this._t && other._b < this._b) ||
      (other._t <= this._t && other._b >= this._b);
    return xok && yok;
  },
  
  round: function(scale) {
    this._l = Math.floor(this._l * scale) / scale;
    this._t = Math.floor(this._t * scale) / scale;
    this._r = Math.ceil(this._r * scale) / scale;
    this._b = Math.ceil(this._b * scale) / scale;
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

  
  
  
  handleEvents: function () {
    let self = this;

    let e = window;
    e.addEventListener("mousedown", function (ev) { return self._onMouseDown(ev); }, true);
    e.addEventListener("mouseup", function (ev) { return self._onMouseUp(ev); }, true);
    e.addEventListener("mousemove", function (ev) { return self._onMouseMove(ev); }, true);
  },

  
  
  
  
  moveWidgetBy: function (wid, x, y) {
    let state = this._getState(wid);

    state.rect.x += x;
    state.rect.y += y;

    this._commitState(state);
  },

  
  
  
  
  
  
  panBy: function panBy(dx, dy, ignoreBarriers) {
    if (dx == 0 && dy ==0)
      return;

    let needsDragWrap = !this._dragging;

    if (needsDragWrap)
      this.dragStart(0, 0);

    this._panBy(dx, dy, ignoreBarriers);

    if (needsDragWrap)
      this.dragStop();
  },

  
  
  panTo: function (x, y) {
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

  
  get viewportVisibleRect () {
    let rect = this._viewportBounds.intersect(this._viewingRect);
    if (!rect)
        rect = new wsRect(0, 0, 0, 0);
    return rect;
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

    this._callViewportUpdateHandler(true);
  },

  
  
  
  
  
  
  
  setViewportHandler: function (uh) {
    this._viewportUpdateHandler = uh;
  },

  
  
  
  
  
  setPanHandler: function (uh) {
    this._panHandler = uh;
  },

  
  dragStart: function (clientX, clientY) {
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

  
  dragStop: function () {
    log("(dragStop)");

    if (!this._dragging)
      return;

    if (this._viewportUpdateTimeout != -1)
      clearTimeout(this._viewportUpdateTimeout);

    this._viewportUpdate();

    this._dragState = null;
  },

  
  dragMove: function dragStop(clientX, clientY) {
    if (!this._dragging)
      return;

    log("(dragMove)", clientX, clientY);

    this._dragCoordsFromClient(clientX, clientY);

    this._dragUpdate();

    if (this._viewportUpdateInterval != -1) {
      if (this._viewportUpdateTimeout != -1)
        clearTimeout(this._viewportUpdateTimeout);
      let self = this;
      this._viewportUpdateTimeout = setTimeout(function () { self._viewportUpdate(); }, this._viewportUpdateInterval);
    }
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
    if (!this._skipViewportUpdates)
      this._startViewportBoundsString = this._viewportBounds.toString();
    this._skipViewportUpdates++;
  },
  
  endUpdateBatch: function endUpdate() {
    if (!this._skipViewportUpdates)
      throw new Error("Unbalanced call to endUpdateBatch");
    this._skipViewportUpdates--;
    if (this._skipViewportUpdates)
      return
    
    let boundsSizeChanged =
      this._startViewportBoundsString != this._viewportBounds.toString();
    this._callViewportUpdateHandler(boundsSizeChanged);
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

  _onMouseDown: function (ev) {
    log("(mousedown)");
    this.dragStart(ev.screenX, ev.screenY);

    
    
    this._dragState.dragging = false;

    let self = this;
    this._dragState.dragStartTimeout = setTimeout(function () {
                                                    self._dragState.dragStartTimeout = -1;
                                                    self._delayedDragStart();
                                                    self._dragUpdate();
                                                  }, 50);
  },

  _onMouseUp: function (ev) {
    log("(mouseup)");
    if (!this._dragState)
      return;

    if (this._dragState.dragStartTimeout != -1)
      clearTimeout(this._dragState.dragStartTimeout);

    this.dragStop();
  },

  _onMouseMove: function (ev) {
    if (!this._dragging)
      return;

    this._dragCoordsFromClient(ev.screenX, ev.screenY);

    if (!this._dragging && this._dragState.outerDistSquared > 100)
      this._delayedDragStart();

    this.dragMove(ev.screenX, ev.screenY);
  },

  get _dragging() {
    return this._dragState && this._dragState.dragging;
  },

  
  _delayedDragStart: function () {
    log("(dragStart)");
    if (this._dragging)
      return;

    if (this._dragState.dragStartTimeout != -1)
      clearTimeout(this._dragState.dragStartTimeout);

    this._dragState.dragging = true;
  },

  _viewportUpdate: function _viewportUpdate() {
    if (!this._viewport)
      return;

    this._viewportUpdateTimeout = -1;

    let vws = this._viewport;
    let vwib = vws.viewportInnerBounds;
    let vpb = this._viewportBounds;

    
    
    
    let [ignoreX, ignoreY] = this._offsets || [0, 0];
    let rx = (vws.dragStartRect.x - vws.rect.x) - ignoreX;
    let ry = (vws.dragStartRect.y - vws.rect.y) - ignoreY;

    let [dX, dY] = this._rectTranslateConstrain(rx, ry, vwib, vpb);

    
    
    this._offsets = [dX - rx, dY - ry];

    
    vwib.translate(dX, dY);
    vws.rect.translate(dX, dY);
    this._commitState(vws);

    
    
    vws.dragStartRect = vws.rect.clone();

    this._callViewportUpdateHandler(false);
  },

  _callViewportUpdateHandler: function _callViewportUpdateHandler(boundsChanged) {
    if (!this._viewport || !this._viewportUpdateHandler || this._skipViewportUpdates)
      return;

    let vwib = this._viewport.viewportInnerBounds.clone();

    vwib.left += this._viewport.offsetLeft;
    vwib.top += this._viewport.offsetTop;
    vwib.right += this._viewport.offsetRight;
    vwib.bottom += this._viewport.offsetBottom;

    this._viewportUpdateHandler.apply(window, [vwib, boundsChanged]);
  },

  _dragCoordsFromClient: function (cx, cy, t) {
    this._dragState.curTime = t ? t : Date.now();
    this._dragState.outerCurX = cx;
    this._dragState.outerCurY = cy;

    let dx = this._dragState.outerCurX - this._dragState.outerStartX;
    let dy = this._dragState.outerCurY - this._dragState.outerStartY;
    this._dragState.outerDX = dx;
    this._dragState.outerDY = dy;
    this._dragState.outerDistSquared = dx*dx + dy*dy;
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
      return;

    
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
  },

  _dragUpdate: function () {
    let dx = this._dragState.outerLastUpdateDX - this._dragState.outerDX;
    let dy = this._dragState.outerLastUpdateDY - this._dragState.outerDY;

    this._dragState.outerLastUpdateDX = this._dragState.outerDX;
    this._dragState.outerLastUpdateDY = this._dragState.outerDY;

    this.panBy(dx, dy);
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
       Math.min(ofRect.top, 0),
       Math.min(ofRect.left, 0),
       Math.max(ofRect.bottom - vp.rect.height, 0),
       Math.max(ofRect.right - vp.rect.width, 0)
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
      intersection = rect.intersect(bounds);
      newIntersection = rect.clone().translate(dx, dy).intersect(bounds);
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
