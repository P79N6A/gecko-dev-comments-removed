











































const AnimatedZoom = {
  startScale: null,

  
  animateTo: function(aZoomRect) {
    if (!aZoomRect)
      return;

    this.zoomTo = aZoomRect.clone();

    if (this.animationDuration === undefined)
      this.animationDuration = Services.prefs.getIntPref("browser.ui.zoom.animationDuration");

    Browser.hideSidebars();
    Browser.hideTitlebar();
    Browser.forceChromeReflow();

    this.start();

    
    if (!this.zoomRect) {
      this.updateTo(this.zoomFrom);

      mozRequestAnimationFrame(this);

      let event = document.createEvent("Events");
      event.initEvent("AnimatedZoomBegin", true, true);
      window.dispatchEvent(event);
    }
  },

  start: function start() {
    this.browser = getBrowser();
    this.zoomFrom = this.zoomRect || this.getStartRect();
    this.startScale = this.browser.scale;
    this.beginTime = mozAnimationStartTime;
  },

  
  getStartRect: function getStartRect() {
    let browser = this.browser;
    let bcr = browser.getBoundingClientRect();
    let scroll = browser.getRootView().getPosition();
    return new Rect(scroll.x, scroll.y, bcr.width, bcr.height);
  },

  
  updateTo: function(nextRect) {
    let zoomRatio = window.innerWidth / nextRect.width;
    let scale = this.startScale * zoomRatio;
    let scrollX = nextRect.left * zoomRatio;
    let scrollY = nextRect.top * zoomRatio;

    this.browser.fuzzyZoom(scale, scrollX, scrollY);

    this.zoomRect = nextRect;
  },

  
  finish: function() {
    this.updateTo(this.zoomTo || this.zoomRect);
    this.browser.finishFuzzyZoom();

    Browser.hideSidebars();
    Browser.hideTitlebar();

    this.beginTime = null;
    this.zoomTo = null;
    this.zoomFrom = null;
    this.zoomRect = null;
    this.startScale = null;

    let event = document.createEvent("Events");
    event.initEvent("AnimatedZoomEnd", true, true);
    window.dispatchEvent(event);
  },

  isZooming: function isZooming() {
    return this.beginTime != null;
  },

  onBeforePaint: function(aTimeStamp) {
    try {
      let tdiff = aTimeStamp - this.beginTime;
      let counter = tdiff / this.animationDuration;
      if (counter < 1) {
        
        let rect = this.zoomFrom.blend(this.zoomTo, counter);
        this.updateTo(rect);
        mozRequestAnimationFrame(this);
      } else {
        
        this.finish();
      }
    } catch(e) {
      this.finish();
      throw e;
    }
  }
};
