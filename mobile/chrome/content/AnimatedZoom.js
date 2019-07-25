








































let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;






function AnimatedZoom(aBrowserView) {
  this.bv = aBrowserView;

  
  let [w, h] = this.bv.getViewportDimensions();
  let viewportRect = new Rect(0, 0, w, h);
  this.zoomFrom = this.bv.getVisibleRect().translateInside(viewportRect);

  
  this.snapshotRect = this.bv.getVisibleRect().inflate(2);

  
  this.snapshotRect.translateInside(viewportRect).restrictTo(viewportRect).expandToIntegers();
  this.snapshot = this._createCanvas(this.snapshotRect.width, this.snapshotRect.height);
  let snapshotCtx = this.snapshot.getContext("2d");
  snapshotCtx.clearRect(0, 0, this.snapshotRect.width, this.snapshotRect.height);
  this.bv.renderToCanvas(this.snapshot, this.snapshotRect.width, this.snapshotRect.height, this.snapshotRect.clone());

  
  this.bv.pauseRendering();

  
  Browser.hideTitlebar();
  Browser.hideSidebars();

  let clientVis = Browser.browserViewToClientRect(this.bv.getCriticalRect());
  let viewBuffer = Elements.viewBuffer;
  viewBuffer.left = clientVis.left;
  viewBuffer.top = clientVis.top;
  viewBuffer.width = this.zoomFrom.width;
  viewBuffer.height = this.zoomFrom.height;
  viewBuffer.style.display = "block";

  
  let ctx = viewBuffer.getContext("2d");

  
  ctx.mozImageSmoothingEnabled = false;
  ctx.globalCompositeOperation = 'copy';

  
  let backgroundImage = new Image();
  backgroundImage.src = "chrome://browser/content/checkerboard.png";
  ctx.fillStyle = ctx.createPattern(backgroundImage, 'repeat');

  
  this.updateTo(this.zoomFrom);
}


AnimatedZoom.prototype._createCanvas = function(width, height) {
  let canvas = document.createElementNS(kXHTMLNamespaceURI, "canvas");
  canvas.width = width;
  canvas.height = height;
  canvas.mozOpaque = true;
  return canvas;
};


AnimatedZoom.prototype.updateTo = function(nextRect) {
  this.zoomRect = nextRect;

  
  let canvasRect = new Rect(0, 0, Elements.viewBuffer.width, Elements.viewBuffer.height);
  let ctx = Elements.viewBuffer.getContext("2d");
  ctx.save();

  
  let srcRect = nextRect.intersect(this.snapshotRect);
  if (srcRect.isEmpty())
    return;

  
  
  let s = canvasRect.width / nextRect.width;
  let destRect = srcRect.clone().translate(-nextRect.x, -nextRect.y).scale(s, s);

  
  srcRect.translate(-this.snapshotRect.left, -this.snapshotRect.top);

  
  destRect.restrictTo(canvasRect).expandToIntegers();
  ctx.drawImage(this.snapshot,
                Math.floor(srcRect.left), Math.floor(srcRect.top),
                Math.floor(srcRect.width), Math.floor(srcRect.height),
                Math.floor(destRect.left), Math.floor(destRect.top),
                Math.floor(destRect.width), Math.floor(destRect.height));

  
  let unknowns = canvasRect.subtract(destRect);
  if (unknowns.length > 0) {
    ctx.beginPath();
    unknowns.forEach(function(r) { ctx.rect(r.x, r.y, r.width, r.height); });
    ctx.clip();
    ctx.fill();
  }

  ctx.restore();
};


AnimatedZoom.prototype.animateTo = function(aZoomRect) {
  if (this.timer)
    return false;

  this.zoomTo = aZoomRect;

  
  this.counter = 0;
  this.inc = 1.0 / Services.prefs.getIntPref("browser.ui.zoom.animationDuration");
  this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this.interval = 1000 / Services.prefs.getIntPref("browser.ui.zoom.animationFps");
  this.timer.initWithCallback(Util.bind(this._callback, this), this.interval, this.timer.TYPE_REPEATING_PRECISE);

  
  this.lastTime = 0;
  return true;
};


AnimatedZoom.prototype._callback = function() {
  try {
    if (this.counter < 1) {
      
      let now = Date.now();
      if (this.lastTime == 0)
        this.lastTime = now - this.interval; 
      this.counter += this.inc * (now - this.lastTime);
      this.lastTime = now;

      
      let rect = this.zoomFrom.blend(this.zoomTo, Math.min(this.counter, 1));
      this.updateTo(rect);
    }
    else {
      
      this.finish();
    }
  }
  catch(e) {
    Util.dumpLn("Error while zooming. Please report error at:", e.getSource());
    this.finish();
    throw e;
  }
};


AnimatedZoom.prototype.finish = function() {
  try {
    
    this.bv.resumeRendering(true);

    
    if (this.zoomRect)
      Browser.setVisibleRect(this.zoomRect);
  }
  finally {
    if (this.timer) {
      this.timer.cancel();
      this.timer = null;
    }
    this.snapshot = null;
  }
};

