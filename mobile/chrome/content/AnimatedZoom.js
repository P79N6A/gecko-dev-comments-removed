








































let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;




function AnimatedZoom() {
  
  this.animationDuration = Services.prefs.getIntPref("browser.ui.zoom.animationDuration");
}

AnimatedZoom.prototype = {
  startTimer: function() {
    if (this.zoomTo && !this.beginTime) {
      Browser.hideSidebars();
      Browser.hideTitlebar();
      Browser.forceChromeReflow();

      let browserRect = Rect.fromRect(getBrowser().getBoundingClientRect());
      this.zoomFrom = browserRect.translate(getBrowser()._frameLoader.viewportScrollX, getBrowser()._frameLoader.viewportScrollY);

      this.updateTo(this.zoomFrom);
      this.beginTime = Date.now();
      window.addEventListener("MozBeforePaint", this, false);
      mozRequestAnimationFrame();
    }
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
  },

  
  updateTo: function(nextRect) {
    let zoomRatio = window.innerWidth / nextRect.width;
    let zoomLevel = getBrowser().scale * zoomRatio;
    
    
    
    
    
    getBrowser()._frameLoader.setViewportScale(zoomLevel, zoomLevel);
    getBrowser()._frameLoader.scrollViewportTo(nextRect.left * zoomRatio, nextRect.top * zoomRatio);
  },

  
  animateTo: function(aZoomRect) {
    this.zoomTo = aZoomRect.clone();
    this.startTimer();
  },

  
  finish: function() {
    window.removeEventListener("MozBeforePaint", this, false);
    Browser.setVisibleRect(this.zoomTo);
    this.beginTime = null;
    this.zoomTo = null;
  }
};
