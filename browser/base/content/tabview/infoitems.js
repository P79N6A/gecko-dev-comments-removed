









































(function() {
















window.InfoItem = function(bounds, options) {
  try {
    Utils.assertThrow('bounds', Utils.isRect(bounds));

    if (typeof(options) == 'undefined')
      options = {};

    this._inited = false;
    this.isAnInfoItem = true;
    this.defaultSize = bounds.size();
    this.locked = (options.locked ? Utils.copy(options.locked) : {});
    this.bounds = new Rect(bounds);
    this.isDragging = false;

    var self = this;

    var $container = iQ('<div>')
      .addClass('info-item')
      .css(this.bounds)
      .appendTo('body');

    this.$contents = iQ('<div>')
      .appendTo($container);

    var $close = iQ('<div>')
      .addClass('close')
      .click(function() {
        self.close();
      })
      .appendTo($container);

    
    if (this.locked.bounds)
      $container.css({cursor: 'default'});

    if (this.locked.close)
      $close.hide();

    
    this._init($container[0]);

    if (this.$debug)
      this.$debug.css({zIndex: -1000});

    
    if (!this.locked.bounds)
      this.draggable();

    
    this.snap();

    
    if (!options.dontPush)
      this.pushAway();

    this._inited = true;
    this.save();
  } catch(e) {
    Utils.log(e);
  }
};


window.InfoItem.prototype = Utils.extend(new Item(), new Subscribable(), {

  
  
  
  getStorageData: function() {
    var data = null;

    try {
      data = {
        bounds: this.getBounds(),
        locked: Utils.copy(this.locked)
      };
    } catch(e) {
      Utils.log(e);
    }

    return data;
  },

  
  
  
  save: function() {
    try {
      if (!this._inited) 
        return;

      var data = this.getStorageData();
  



    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  setBounds: function(rect, immediately) {
    try {
      Utils.assertThrow('InfoItem.setBounds: rect must be a real rectangle!', Utils.isRect(rect));

      
      var css = {};

      if (rect.left != this.bounds.left)
        css.left = rect.left;

      if (rect.top != this.bounds.top)
        css.top = rect.top;

      if (rect.width != this.bounds.width)
        css.width = rect.width;

      if (rect.height != this.bounds.height)
        css.height = rect.height;

      if (Utils.isEmptyObject(css))
        return;

      this.bounds = new Rect(rect);
      Utils.assertThrow('InfoItem.setBounds: this.bounds must be a real rectangle!',
                        Utils.isRect(this.bounds));

      
      if (immediately) {
        iQ(this.container).css(css);
      } else {
        TabItems.pausePainting();
        iQ(this.container).animate(css, {
          duration: 350,
          easing: "tabviewBounce",
          complete: function() {
            TabItems.resumePainting();
          }
        });
      }

      this._updateDebugBounds();
      this.setTrenches(rect);
      this.save();
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  setZ: function(value) {
    try {
      Utils.assertThrow('value must be a number', typeof(value) == 'number');

      this.zIndex = value;

      iQ(this.container).css({zIndex: value});

      if (this.$debug)
        this.$debug.css({zIndex: value + 1});
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  close: function() {
    try {
      this._sendToSubscribers("close");
      this.removeTrenches();
      iQ(this.container).fadeOut(function() {
        iQ(this).remove();
        Items.unsquish();
      });

  
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  html: function(value) {
    try {
      Utils.assertThrow('value must be a string', typeof(value) == 'string');
      this.$contents.html(value);
    } catch(e) {
      Utils.log(e);
    }
  }
});

})();
