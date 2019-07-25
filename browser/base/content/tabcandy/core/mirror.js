









































(function(){



function _isIframe(doc){
  var win = doc.defaultView;
  return win.parent != win;
}




var TabCanvas = function(tab, canvas){
  this.init(tab, canvas);
};

TabCanvas.prototype = {
  
  
  init: function(tab, canvas){
    this.tab = tab;
    this.canvas = canvas;
    this.window = window;

    var $canvas = iQ(canvas).data("link", this);

    var w = $canvas.width();
    var h = $canvas.height();
    canvas.width = w;
    canvas.height = h;

    var self = this;
    this.paintIt = function(evt) {
      
      self.tab.mirror.triggerPaint();

    };
  },

  
  
  attach: function() {
    this.tab.contentWindow.addEventListener("MozAfterPaint", this.paintIt, false);
  },

  
  
  detach: function() {
    try {
      this.tab.contentWindow.removeEventListener("MozAfterPaint", this.paintIt, false);
    } catch(e) {
      
    }
  },

  
  
  paint: function(evt){
    var ctx = this.canvas.getContext("2d");

    var w = this.canvas.width;
    var h = this.canvas.height;
    if (!w || !h)
      return;

    var fromWin = this.tab.contentWindow;
    if (fromWin == null) {
      Utils.log('null fromWin in paint');
      return;
    }

    var scaler = w/fromWin.innerWidth;

    

    ctx.save();
    ctx.scale(scaler, scaler);
    try{
      ctx.drawWindow( fromWin, fromWin.scrollX, fromWin.scrollY, w/scaler, h/scaler, "#fff" );
    } catch(e){
      Utils.error('paint', e);
    }

    ctx.restore();
  },

  
  
  toImageData: function() {
    return this.canvas.toDataURL("image/png", "");
  }
};





function Mirror(tab, manager) {

  this.tab = tab;
  this.manager = manager;

  var $div = iQ('<div>')
    .data("tab", this.tab)
    .addClass('tab')
    .html("<div class='favicon'><img/></div>" +
          "<div class='thumb'><div class='thumb-shadow'></div>" +
          "<img class='cached-thumb' style='display:none'/><canvas/></div>" +
          "<span class='tab-title'>&nbsp;</span>"
    )
    .appendTo('body');

  this.needsPaint = 0;
  this.canvasSizeForced = false;
  this.isShowingCachedData = false;
  this.el = $div.get(0);
  this.favEl = iQ('.favicon>img', $div).get(0);
  this.nameEl = iQ('.tab-title', $div).get(0);
  this.canvasEl = iQ('.thumb canvas', $div).get(0);
  this.cachedThumbEl = iQ('img.cached-thumb', $div).get(0);
  this.okayToHideCache = false;

  var doc = this.tab.contentDocument;
  if ( !_isIframe(doc) ) {
    this.tabCanvas = new TabCanvas(this.tab, this.canvasEl);
    this.tabCanvas.attach();
    this.triggerPaint();
  }


  this.tab.mirror = this;
  this.manager.createTabItem(this);

}

Mirror.prototype = iQ.extend(new Subscribable(), {
  
  
  
  triggerPaint: function() {
    var date = new Date();
    this.needsPaint = date.getTime();
  },

  
  
  
  
  forceCanvasSize: function(w, h) {
    this.canvasSizeForced = true;
    this.canvasEl.width = w;
    this.canvasEl.height = h;
    this.tabCanvas.paint();
  },

  
  
  
  
  
  
  unforceCanvasSize: function() {
    this.canvasSizeForced = false;
  },

  
  
  
  
  showCachedData: function(tabData) {
    this.isShowingCachedData = true;
    var $nameElement = iQ(this.nameEl);
    var $canvasElement = iQ(this.canvasEl);
    var $cachedThumbElement = iQ(this.cachedThumbEl);
    $cachedThumbElement.attr("src", tabData.imageData).show();
    $canvasElement.css({opacity: 0.0});
    $nameElement.text(tabData.title ? tabData.title : "");
  },

  
  
  
  hideCachedData: function() {
    var $canvasElement = iQ(this.canvasEl);
    var $cachedThumbElement = iQ(this.cachedThumbEl);
    $cachedThumbElement.hide();
    $canvasElement.css({opacity: 1.0});
  }
});




var TabMirror = function() {
  if (window.Tabs) {
    this.init();
  }
  else {
    var self = this;
    TabsManager.addSubscriber(this, 'load', function() {
      self.init();
    });
  }
};

TabMirror.prototype = {
  
  
  
  init: function(){
    var self = this;
    
    
    Tabs.onOpen(function() {
      var tab = this;
      iQ.timeout(function() { 
        self.update(tab);
      }, 1);
    });

    
    Tabs.onReady(function(evt) {
      var tab = evt.tab;
      iQ.timeout(function() { 
        self.update(tab);
      }, 1);
    });

    
    
    Tabs.onLoad(function(evt) {
      var tab = evt.tab;
      iQ.timeout(function() { 
        tab.mirror.okayToHideCache = true;
        self.update(tab);
      }, 1);
    });

    
    Tabs.onClose( function(){
      var tab = this;
      iQ.timeout(function() { 
        self.unlink(tab);
      }, 1);
    });

    
    Tabs.forEach(function(tab){
      self.link(tab);
    });

    this.paintingPaused = 0;
    this.heartbeatIndex = 0;
    this._fireNextHeartbeat();
  },

  
  
  _heartbeat: function() {
    try {
      var now = + new Date; 
      var count = Tabs.length;
      if (count && this.paintingPaused <= 0) {
        this.heartbeatIndex++;
        if (this.heartbeatIndex >= count)
          this.heartbeatIndex = 0;

        var tab = Tabs[this.heartbeatIndex];
        var mirror = tab.mirror;
        if (mirror) {
          var iconUrl = tab.raw.linkedBrowser.mIconURL;
          if ( iconUrl == null ){
            iconUrl = "chrome://mozapps/skin/places/defaultFavicon.png";
          }

          var label = tab.raw.label;
          var $name = iQ(mirror.nameEl);
          var $canvas = iQ(mirror.canvasEl);

          if (iconUrl != mirror.favEl.src) {
            mirror.favEl.src = iconUrl;
            mirror.triggerPaint();
          }

          if (tab.url != mirror.url) {
            var oldURL = mirror.url;
            mirror.url = tab.url;
            mirror._sendToSubscribers(
              'urlChanged', {oldURL: oldURL, newURL: tab.url});
            mirror.triggerPaint();
          }

          if (!mirror.isShowingCachedData && $name.text() != label) {
            $name.text(label);
            mirror.triggerPaint();
          }

          if (!mirror.canvasSizeForced) {
            var w = $canvas.width();
            var h = $canvas.height();
            if (w != mirror.canvasEl.width || h != mirror.canvasEl.height) {
              mirror.canvasEl.width = w;
              mirror.canvasEl.height = h;
              mirror.triggerPaint();
            }
          }

          if (mirror.needsPaint) {
            mirror.tabCanvas.paint();

            if (mirror.isShowingCachedData && mirror.okayToHideCache) 
              mirror.hideCachedData();

						
            if ((+ new Date) - mirror.needsPaint > 5000) 
              mirror.needsPaint = 0;
          }
        }
      }
    } catch(e) {
      Utils.error('heartbeat', e);
    }

    this._fireNextHeartbeat();
  },

  
  
  _fireNextHeartbeat: function() {
    var self = this;
    iQ.timeout(function() {
      self._heartbeat();
    }, 100);
  },

  
  
  createTabItem: function(mirror) {
    try {
      var $div = iQ(mirror.el);
      var tab = mirror.tab;
      var item = new window.TabItem(mirror.el, tab);

      item.addOnClose(window.TabItems, function() {
        Items.unsquish(null, item);
      });

      if (!window.TabItems.reconnect(item))
        Groups.newTab(item);
    } catch(e) {
      Utils.error(e);
    }
  },

  
  
  update: function(tab){
    this.link(tab);

    if (tab.mirror && tab.mirror.tabCanvas)
      tab.mirror.triggerPaint();
  },

  
  
  link: function(tab){
    
    if (tab.mirror)
      return false;

    
    new Mirror(tab, this); 
    return true;
  },

  
  
  unlink: function(tab){
    var mirror = tab.mirror;
    if (mirror) {
      mirror._sendToSubscribers("close");
      var tabCanvas = mirror.tabCanvas;
      if (tabCanvas)
        tabCanvas.detach();

      iQ(mirror.el).remove();

      tab.mirror = null;
    }
  }
};


window.TabMirror = {
  _private: new TabMirror(),

  
  
  
  
  
  pausePainting: function() {
    this._private.paintingPaused++;
  },

  
  
  
  
  resumePainting: function() {
    this._private.paintingPaused--;
  },

  
  
  
  isPaintingPaused: function() {
    return this._private.paintingPause > 0;
  }
};

})();
