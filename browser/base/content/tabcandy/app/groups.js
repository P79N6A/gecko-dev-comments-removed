(function(){

var numCmp = function(a,b){ return a-b; }

function min(list){ return list.slice().sort(numCmp)[0]; }
function max(list){ return list.slice().sort(numCmp).reverse()[0]; }

function isEventOverElement(event, el){
  var hit = {nodeName: null};
  var isOver = false;
  
  var hiddenEls = [];
  while(hit.nodeName != "BODY" && hit.nodeName != "HTML"){
    hit = document.elementFromPoint(event.clientX, event.clientY);
    if( hit == el ){
      isOver = true;
      break;
    }
    $(hit).hide();
    hiddenEls.push(hit);
  }
  
  var hidden;
  [$(hidden).show() for([,hidden] in Iterator(hiddenEls))];
  return isOver;
}


window.Group = function(listOfEls, options) {
  if(typeof(options) == 'undefined')
    options = {};

  this._children = []; 
  this._container = null;
  this._padding = 30;
  this.defaultSize = new Point(TabItems.tabWidth * 1.5, TabItems.tabHeight * 1.5);

  var self = this;

  var boundingBox = this._getBoundingBox(listOfEls);
  var padding = 30;
  var container = $("<div class='group'/>")
    .css({
      position: "absolute",
      top: boundingBox.top-padding,
      left: boundingBox.left-padding,
      width: boundingBox.width+padding*2,
      height: boundingBox.height+padding*2,
      zIndex: -100,
      opacity: 0,
    })
    .data('item', this)
    .appendTo("body")
    .animate({opacity:1.0}).dequeue();
  




  
  var resizer = $("<div class='resizer'/>")
    .css({
      position: "absolute",
      width: 16, height: 16,
      bottom: 0, right: 0,
    }).appendTo(container);

  var titlebar = $("<div class='titlebar'><input class='name' value=''/><div class='close'>x</div></div>")
    .appendTo(container)
  
  titlebar.css({
      width: container.width(),
      position: "relative",
      top: -(titlebar.height()+2),
      left: -1,
    });
    
  $('.close', titlebar).click(function() {
    self.close();
  });

  
  var shouldShow = false;
  container.mouseover(function(){
    shouldShow = true;
    setTimeout(function(){
      if( shouldShow == false ) return;
      container.find("input").focus();
      titlebar
        .css({width: container.width()})
        .animate({ opacity: 1}).dequeue();        
    }, 500);
  }).mouseout(function(e){
    shouldShow = false;
    if( isEventOverElement(e, container.get(0) )) return;
    titlebar.animate({opacity:0}).dequeue();
  })

  this._container = container;

  $.each(listOfEls, function(index, el) {  
    self.add(el, null, options);
  });

  this._addHandlers(container);
  
  Groups.register(this);
  
  
  if(!options.dontPush)
    this.pushAway();   
};


window.Group.prototype = $.extend(new Item(), new Subscribable(), {  
  
  _getBoundingBox: function(els) {
    var el;
    var boundingBox = {
      top:    min( [$(el).position().top  for([,el] in Iterator(els))] ),
      left:   min( [$(el).position().left for([,el] in Iterator(els))] ),
      bottom: max( [$(el).position().top  for([,el] in Iterator(els))] )  + $(els[0]).height(),
      right:  max( [$(el).position().left for([,el] in Iterator(els))] ) + $(els[0]).width(),
    };
    boundingBox.height = boundingBox.bottom - boundingBox.top;
    boundingBox.width  = boundingBox.right - boundingBox.left;
    return boundingBox;
  },
  
  
  _getContainerBox: function(){
    var pos = $(this._container).position();
    var w = $(this._container).width();
    var h = $(this._container).height();
    return {
      top: pos.top,
      left: pos.left,
      bottom: pos.top + h,
      right: pos.left + w,
      height: h,
      width: w
    }
  },
  
  
  getContainer: function() {
    return this._container;
  },

  
  getBounds: function() {
    var $titlebar = $('.titlebar', this._container);
    var bb = Utils.getBounds(this._container);
    var titleHeight = $titlebar.height();
    bb.top -= titleHeight;
    bb.height += titleHeight;
    return bb;
  },
  
  
  setBounds: function(rect, immediately) {
    this.setPosition(rect.left, rect.top, immediately);
    this.setSize(rect.width, rect.height, immediately, {dontArrange: true});

    var $titlebar = $('.titlebar', this._container);
    var titleHeight = $titlebar.height();
    var box = new Rect(rect);
    box.top += titleHeight;
    box.height -= titleHeight;
    this.arrange({animate: !immediately, bounds: box});
  },
  
  
  setPosition: function(left, top, immediately) {
    var box = this.getBounds();
    var offset = new Point(left - box.left, top - box.top);
    
    $.each(this._children, function(index, child) {
      box = child.getBounds();
      child.setPosition(box.left + offset.x, box.top + offset.y, immediately);
    });
        
    box = Utils.getBounds(this._container);
    var options = {left: box.left + offset.x, top: box.top + offset.y};
    if(immediately)
      $(this._container).css(options);
    else
      $(this._container).animate(options).dequeue();
  },

  
  setSize: function(width, height, immediately, options) {
    if(typeof(options) == 'undefined')
      options = {};
      
    var $titlebar = $('.titlebar', this._container);
    var titleHeight = $titlebar.height();
    
    var containerOptions = {width: width, height: height - titleHeight};
    var titleOptions = {width: width};
    if(immediately) {
      $(this._container).css(containerOptions);
      $titlebar.css(titleOptions);
    } else {
      $(this._container).animate(containerOptions).dequeue();
      $titlebar.animate(titleOptions).dequeue();
    }
    
    if(!options.dontArrange) {
      
      var box = this.getBounds();
      box.width = width;
      box.top += titleHeight;
      box.height = height - titleHeight;
      this.arrange({animate: !immediately, bounds: box});
    }
  },

  
  close: function() {
    var toClose = $.merge([], this._children);
    $.each(toClose, function(index, child) {
      child.close();
    });
    
    this._sendOnClose();
  },
  
  
  add: function($el, dropPos, options) {
    Utils.assert('add expects jQuery objects', Utils.isJQuery($el));
    
    if(!dropPos) 
      dropPos = {top:window.innerWidth, left:window.innerHeight};
      
    if(typeof(options) == 'undefined')
      options = {};
      
    var self = this;
    
    
    
    function findInsertionPoint(dropPos){
      var best = {dist: Infinity, item: null};
      var index = 0;
      var box;
      for each(var child in self._children){
        box = child.getBounds();
        var dist = Math.sqrt( Math.pow((box.top+box.height/2)-dropPos.top,2) + Math.pow((box.left+box.width/2)-dropPos.left,2) );

        if( dist <= best.dist ){
          best.item = child;
          best.dist = dist;
          best.index = index;
        }
        index += 1;
      }

      if( self._children.length > 0 ){
        box = best.item.getBounds();
        var insertLeft = dropPos.left <= box.left + box.width/2;
        if( !insertLeft ) 
          return best.index+1;
        else 
          return best.index;
      }
      
      return 0;      
    }
    
    var item = Items.item($el);
    var oldIndex = $.inArray(item, this._children);
    if(oldIndex != -1)
      this._children.splice(oldIndex, 1); 

    var index = findInsertionPoint(dropPos);
    this._children.splice( index, 0, item );

    $el.droppable("disable");

    item.addOnClose(this, function() {
      self.remove($el);
    });      
    
    $el.data("group", this);
    
    if(!options.dontArrange)
      this.arrange();
  },
  
  
  
  remove: function(a, options) {
    var $el;  
    var item;

    if(a.isAnItem) {
      item = a;
      $el = $(item.getContainer());
    } else {
      $el = $(a);  
      item = Items.item($el);
    }
    
    if(typeof(options) == 'undefined')
      options = {};
    
    var index = $.inArray(item, this._children);
    if(index != -1)
      this._children.splice(index, 1); 

    $el.data("group", null);
    item.setSize(item.defaultSize.x, item.defaultSize.y);
    $el.droppable("enable");    
    item.removeOnClose(this);
    
    if(this._children.length == 0 ){
      Groups.unregister(this);
      this._container.fadeOut(function() {
        $(this).remove();
      });
    } else if(!options.dontArrange) {
      this.arrange();
    }
  },
  
  
  removeAll: function() {
    var self = this;
    var toRemove = $.merge([], this._children);
    $.each(toRemove, function(index, child) {
      self.remove(child, {dontArrange: true});
    });
  },
    
  
  arrange: function(options) {
    if( options && options.animate == false ) 
      animate = false;
    else 
      animate = true;

    if(typeof(options) == 'undefined')
      options = {};
    
    var bb = (options.bounds ? options.bounds : this._getContainerBox());

    var count = this._children.length;
    var bbAspect = bb.width/bb.height;
    var tabAspect = 4/3; 
    
    function howManyColumns( numRows, count ){ return Math.ceil(count/numRows) }
    
    var count = this._children.length;
    var best = {cols: 0, rows:0, area:0};
    for(var numRows=1; numRows<=count; numRows++){
      numCols = howManyColumns( numRows, count);
      var w = numCols*tabAspect;
      var h = numRows;
      
      
      if( w/bb.width >= h/bb.height ) var scale = bb.width/w;
      
      else var scale = bb.height/h;
      var w = w*scale;
      var h = h*scale;
            
      if( w*h >= best.area ){
        best.numRows = numRows;
        best.numCols = numCols;
        best.area = w*h;
        best.w = w;
        best.h = h;
      }
    }
    
    var padAmount = .1;
    var pad = padAmount * (best.w/best.numCols);
    var tabW = (best.w-pad)/best.numCols - pad;
    var tabH = (best.h-pad)/best.numRows - pad;
    
    var x = pad; var y=pad; var numInCol = 0;
    for each(var item in this._children){
      item.setBounds(new Rect(x + bb.left, y + bb.top, tabW, tabH), !animate);
      
      x += tabW + pad;
      numInCol += 1;
      if( numInCol >= best.numCols ) 
        [x, numInCol, y] = [pad, 0, y+tabH+pad];
    }
  },
  
  
  _addHandlers: function(container){
    var self = this;
    
    $(container).draggable({
      start: function(){
        $dragged = $(this);
        $(container).data("origPosition", $(container).position());
        $.each(self._children, function(index, child) {
          child.dragData = {startBounds: child.getBounds()};
        });
      },
      drag: function(e, ui){
        var origPos = $(container).data("origPosition");
        dX = ui.offset.left - origPos.left;
        dY = ui.offset.top - origPos.top;
        $.each(self._children, function(index, child) {
          child.setPosition(child.dragData.startBounds.left + dX, 
            child.dragData.startBounds.top + dY, 
            true);
        });



      }, 
      stop: function() {
        if(!$dragged.hasClass('willGroup') && !$dragged.data('group'))
          Items.item(this).pushAway();

        $dragged = null; 
      },

    });
    
    $(container).droppable({
      over: function(){
        $dragged.addClass("willGroup");
      },
      out: function(){
        var $group = $dragged.data("group");
        if($group)
          $group.remove($dragged);
        $dragged.removeClass("willGroup");
      },
      drop: function(event){
        $dragged.removeClass("willGroup");
        self.add( $dragged, {left:event.pageX, top:event.pageY} )
      },
      accept: ".tab, .group",
    });
        
    $(container).resizable({
      handles: "se",
      aspectRatio: false,
      resize: function(){
        self.arrange({animate: false});
      },
      stop: function(){
        self.arrange();
        self.pushAway();
      } 
    });
  }
});


var zIndex = 100;
var $dragged = null;
var timeout = null;

window.Groups = {
  groups: [],
  
  
  dragOptions: {
    start:function(){
      $dragged = $(this);
      $dragged.data('isDragging', true);
    },
    stop: function(){
      $dragged.data('isDragging', false);
      $(this).css({zIndex: zIndex});
      if(!$dragged.hasClass('willGroup') && !$dragged.data('group'))
        Items.item(this).pushAway();

      $dragged = null;          
      zIndex += 1;
    },
    zIndex: 999,
  },
  
  
  dropOptions: {
    accept: ".tab",
    tolerance: "pointer",
    greedy: true,
    drop: function(e){
      $target = $(e.target);
  
      
      if( $target.css("zIndex") < $dragged.data("topDropZIndex") ) return;
      $dragged.data("topDropZIndex", $target.css("zIndex") );
      $dragged.data("topDrop", $target);
      
      
      
      
      
      clearTimeout( timeout );
      var dragged = $dragged;
      var target = $target;
      timeout = setTimeout( function(){
        dragged.removeClass("willGroup")   
  
        dragged.animate({
          top: target.position().top+15,
          left: target.position().left+15,      
        }, 100);
        
        setTimeout( function(){
          var group = $(target).data("group");
          if( group == null ){
            var group = new Group([target, dragged]);
          } else {
            group.add( dragged );
          }
          
        }, 100);
        
        
      }, 10 );
      
      
    },
    over: function(e){
      $dragged.addClass("willGroup");
      $dragged.data("topDropZIndex", 0);    
    },
    out: function(){      
      $dragged.removeClass("willGroup");
    }
  }, 
  
  
  register: function(group) {
    Utils.assert('only register once per group', $.inArray(group, this.groups) == -1);
    this.groups.push(group);
  },
  
  
  unregister: function(group) {
    var index = $.inArray(group, this.groups);
    if(index != -1)
      this.groups.splice(index, 1);     
  },
  
  
  arrange: function() {
    var count = this.groups.length;
    var columns = Math.ceil(Math.sqrt(count));
    var rows = ((columns * columns) - count >= columns ? columns - 1 : columns); 
    var padding = 12;
    var startX = padding;
    var startY = Page.startY;
    var totalWidth = window.innerWidth - startX;
    var totalHeight = window.innerHeight - startY;
    var box = new Rect(startX, startY, 
        (totalWidth / columns) - padding,
        (totalHeight / rows) - padding);
    
    $.each(this.groups, function(index, group) {
      group.setBounds(box, true);
      
      box.left += box.width + padding;
      if(index % columns == columns - 1) {
        box.left = startX;
        box.top += box.height + padding;
      }
    });
  },
  
  
  removeAll: function() {
    var toRemove = $.merge([], this.groups);
    $.each(toRemove, function(index, group) {
      group.removeAll();
    });
  }
};


$(".tab").data('isDragging', false)
  .draggable(window.Groups.dragOptions)
  .droppable(window.Groups.dropOptions);

})();