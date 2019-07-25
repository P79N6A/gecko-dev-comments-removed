

























































function InfoItem(bounds, options) {
  try {
    Utils.assertThrow(Utils.isRect(bounds), 'bounds');

    if (typeof options == 'undefined')
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


InfoItem.prototype = Utils.extend(new Item(), new Subscribable(), {

  
  
  
  getStorageData: function InfoItem_getStorageData() {
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

  
  
  
  save: function InfoItem_save() {
    try {
      if (!this._inited) 
        return;

      var data = this.getStorageData();

    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  setBounds: function InfoItem_setBounds(rect, immediately) {
    try {
      Utils.assertThrow(Utils.isRect(rect), 'InfoItem.setBounds: rect must be a real rectangle!');

      
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
      Utils.assertThrow(Utils.isRect(this.bounds), 
          'InfoItem.setBounds: this.bounds must be a real rectangle!');

      
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

  
  
  
  setZ: function InfoItem_setZ(value) {
    try {
      Utils.assertThrow(typeof value == 'number', 'value must be a number');

      this.zIndex = value;

      iQ(this.container).css({zIndex: value});

      if (this.$debug)
        this.$debug.css({zIndex: value + 1});
    } catch(e) {
      Utils.log(e);
    }
  },

  
  
  
  close: function InfoItem_close() {
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

  
  
  
  html: function InfoItem_html(value) {
    try {
      Utils.assertThrow(typeof value == 'string', 'value must be a string');
      this.$contents.html(value);
    } catch(e) {
      Utils.log(e);
    }
  }
});
