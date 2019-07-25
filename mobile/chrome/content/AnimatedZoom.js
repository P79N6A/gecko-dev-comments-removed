











































const AnimatedZoom = {
  
  animateTo: function(aZoomRect) {
    if (!aZoomRect)
      return;

    this.zoomTo = aZoomRect.clone();

    if (this.animationDuration === undefined)
      this.animationDuration = Services.prefs.getIntPref("browser.ui.zoom.animationDuration");

    Browser.hideSidebars();
    Browser.hideTitlebar();
    Browser.forceChromeReflow();

    this.beginTime = mozAnimationStartTime;

    
    if (this.zoomRect) {
      this.zoomFrom = this.zoomRect;
    } else {
      this.zoomFrom = this.getStartRect();
      this.updateTo(this.zoomFrom);

      window.addEventListener("MozBeforePaint", this, false);
      mozRequestAnimationFrame();

      let event = document.createEvent("Events");
      event.initEvent("AnimatedZoomBegin", true, true);
      window.dispatchEvent(event);
    }
  },

  
  getStartRect: function getStartRect() {
    let browser = getBrowser();
    let bcr = browser.getBoundingClientRect();
    let scroll = browser.getRootView().getPosition();
    return new Rect(scroll.x, scroll.y, bcr.width, bcr.height);
  },

  
  updateTo: function(nextRect) {
    let browser = getBrowser();
    let zoomRatio = window.innerWidth / nextRect.width;
    let zoomLevel = browser.scale * zoomRatio;

    
    
    let contentView = browser.getRootView();
    contentView.setScale(zoomLevel);
    contentView._contentView.scrollTo(nextRect.left * zoomRatio, nextRect.top * zoomRatio);

    this.zoomRect = nextRect;
  },

  
  finish: function() {
    window.removeEventListener("MozBeforePaint", this, false);
    Browser.setVisibleRect(this.zoomTo || this.zoomRect);
    this.beginTime = null;
    this.zoomTo = null;
    this.zoomFrom = null;
    this.zoomRect = null;

    let event = document.createEvent("Events");
    event.initEvent("AnimatedZoomEnd", true, true);
    window.dispatchEvent(event);
  },

  isZooming: function isZooming() {
    return this.beginTime != null;
  },

  handleEvent: function(aEvent) {
    try {
      let tdiff = aEvent.timeStamp - this.beginTime;
      let counter = tdiff / this.animationDuration;
      if (counter < 1) {
        
        let rect = this.zoomFrom.blend(this.zoomTo, counter);
        this.updateTo(rect);
        mozRequestAnimationFrame();
      } else {
        
        this.finish();
      }
    } catch(e) {
      this.finish();
      throw e;
    }
  }
};
