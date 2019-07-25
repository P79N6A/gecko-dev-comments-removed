








































let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;




const animatedZoom = {
  
  animateTo: function(aZoomRect) {
    if (!aZoomRect)
      return;

    this.zoomTo = aZoomRect.clone();

    if (this.animationDuration === undefined)
      this.animationDuration = Services.prefs.getIntPref("browser.ui.zoom.animationDuration");

    Browser.hideSidebars();
    Browser.hideTitlebar();
    Browser.forceChromeReflow();

    this.beginTime = Date.now();

    
    if (this.zoomRect) {
      this.zoomFrom = this.zoomRect;
    }
    else {
      let browserRect = Rect.fromRect(getBrowser().getBoundingClientRect());
      let scrollX = {}, scrollY = {};
      getBrowser().getPosition(scrollX, scrollY);
      this.zoomFrom = browserRect.translate(scrollX.value, scrollY.value);
      this.updateTo(this.zoomFrom);

      window.addEventListener("MozBeforePaint", this, false);
      mozRequestAnimationFrame();
    }
  },

  
  updateTo: function(nextRect) {
    let zoomRatio = window.innerWidth / nextRect.width;
    let zoomLevel = getBrowser().scale * zoomRatio;
    
    
    
    
    
    getBrowser()._frameLoader.setViewportScale(zoomLevel, zoomLevel);
    getBrowser()._frameLoader.scrollViewportTo(nextRect.left * zoomRatio, nextRect.top * zoomRatio);
    this.zoomRect = nextRect;
  },

  
  finish: function() {
    window.removeEventListener("MozBeforePaint", this, false);
    Browser.setVisibleRect(this.zoomTo || this.zoomRect);
    this.beginTime = null;
    this.zoomTo = null;
    this.zoomFrom = null;
    this.zoomRect = null;
  },

  handleEvent: function(aEvent) {
    try {
      let tdiff = aEvent.timeStamp - this.beginTime;
      let counter = tdiff / this.animationDuration;
      if (counter < 1) {
        
        let rect = this.zoomFrom.blend(this.zoomTo, Math.min(counter, 1));
        this.updateTo(rect);
        mozRequestAnimationFrame();
      }
      else {
        
        this.finish();
      }
    }
    catch(e) {
      this.finish();
      throw e;
    }
  }
};
