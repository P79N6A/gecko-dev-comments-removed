
(function(){

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

function _isIframe(doc){
  var win = doc.defaultView;
  return win.parent != win;
}




var TabCanvas = function(tab, canvas){ this.init(tab, canvas) }
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
    if(!w || !h)
      return;
  
    var fromWin = this.tab.contentWindow;
    if(fromWin == null) {
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
  
  
  
  
  animate: function(options, duration){
    Utils.assert('this routine no longer exists', false);
  }
}





function Mirror(tab, manager) {

  this.tab = tab;
  this.manager = manager;
  
  var $div = iQ('<div>')
    .data("tab", this.tab)
    .addClass('tab')
    .html("<div class='favicon'><img/></div>" +
          "<div class='thumb'><div class='thumbShadow'></div><canvas/></div>" +
          "<span class='tab-title'>&nbsp;</span>"
    )
    .appendTo('body');
    
  this.needsPaint = 0;
  this.canvasSizeForced = false;
  this.el = $div.get(0);
  this.favEl = iQ('.favicon>img', $div).get(0);
  this.nameEl = iQ('.tab-title', $div).get(0);
  this.canvasEl = iQ('.thumb canvas', $div).get(0);
      
  var doc = this.tab.contentDocument;
  if( !_isIframe(doc) ) {
    this.tabCanvas = new TabCanvas(this.tab, this.canvasEl);    
    this.tabCanvas.attach();
    this.triggerPaint();
  }
  

  this.tab.mirror = this;
  this.manager._customize(this);

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
  }
});




var TabMirror = function() {
  if(window.Tabs) 
    this.init();
  else { 
    var self = this;
    TabsManager.addSubscriber(this, 'load', function() {
      self.init();
    });
  }
}

TabMirror.prototype = {
  
  
  
  init: function(){
    var self = this;
        
    
    Tabs.onOpen(function() { 
      var tab = this;
      setTimeout(function() { 
        self.update(tab);
      }, 1);
    });

    
    Tabs.onReady( function(evt){
      var tab = evt.tab;
      setTimeout(function() { 
        self.update(tab);
      }, 1);
    });
    
    
    Tabs.onClose( function(){
      var tab = this;
      setTimeout(function() { 
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

      var now = Utils.getMilliseconds();
      var count = Tabs.length;
      if(count && this.paintingPaused <= 0) {
        this.heartbeatIndex++;
        if(this.heartbeatIndex >= count)
          this.heartbeatIndex = 0;
          
        var tab = Tabs[this.heartbeatIndex];
        var mirror = tab.mirror; 

        if(mirror) {

          var iconUrl = tab.raw.linkedBrowser.mIconURL;
          if( iconUrl == null ){
            iconUrl = "chrome://tabcandy/content/candies/revision-a/img/default.png";
          }

          var label = tab.raw.label;
          $name = iQ(mirror.nameEl);
          $canvas = iQ(mirror.canvasEl);
          
          if(iconUrl != mirror.favEl.src) { 
            mirror.favEl.src = iconUrl;
            mirror.triggerPaint();
          }
            
          if($name.text() != label) {
            $name.text(label);
            mirror.triggerPaint();
          }
          
          if(tab.url != mirror.url) {
            var oldURL = mirror.url;
            mirror.url = tab.url;
            mirror._sendToSubscribers('urlChanged', {oldURL: oldURL, newURL: tab.url});
            mirror.triggerPaint();
          }
          
          if(!mirror.canvasSizeForced) {
            var w = $canvas.width();
            var h = $canvas.height();
            if(w != mirror.canvasEl.width || h != mirror.canvasEl.height) {
              mirror.canvasEl.width = w;
              mirror.canvasEl.height = h;
              mirror.triggerPaint();
            }
          }
          
          if(mirror.needsPaint) {

            mirror.tabCanvas.paint();
            
            if(Utils.getMilliseconds() - mirror.needsPaint > 5000)
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
    window.setTimeout(function() {
      self._heartbeat();
    }, 100);
  },   
    
  _customize: function(func){
    
    
  },
  
  _createEl: function(tab){
    new Mirror(tab, this); 
  },
  
  update: function(tab){
    this.link(tab);

    if(tab.mirror && tab.mirror.tabCanvas)
      tab.mirror.triggerPaint();
  },
  
  link: function(tab){
    
    if(tab.mirror)
      return false;
    
    
    this._createEl(tab);
    return true;
  },
  
  unlink: function(tab){
    var mirror = tab.mirror;
    if(mirror) {
      mirror._sendOnClose();
      var tabCanvas = mirror.tabCanvas;
      if(tabCanvas)
        tabCanvas.detach();
      
      iQ(mirror.el).remove();
      
      tab.mirror = null;
    }
  }  
};


window.TabMirror = {
  _private: new TabMirror(), 
  
  
  
  
  
  
  
  
  customize: function(func) {
    
    iQ('div.tab').each(function() {
      var tab = Tabs.tab(this);
      func(tab.mirror);
    });
    
    
    TabMirror.prototype._customize = func;
  },

  
  
  
  
  
  pausePainting: function() {

  },
  
  
  
  
  
  resumePainting: function() {

  },

  
  
  
  isPaintingPaused: function() {
    return this._private.paintingPause > 0;
  }
};

})();
