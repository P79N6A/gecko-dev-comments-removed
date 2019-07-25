(function(){

var numCmp = function(a,b){ return a-b; }

function min(list){ return list.slice().sort(numCmp)[0]; }
function max(list){ return list.slice().sort(numCmp).reverse()[0]; }

function Group(){}
Group.prototype = {
  _children: [],
  _container: null,
  _padding: 30,
  
  _getBoundingBox: function(){
    var els = this._children;
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
  
  create: function(listOfEls){
    this._children = $(listOfEls).toArray();

    var boundingBox = this._getBoundingBox();
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
      .appendTo("body")
      .animate({opacity:1.0});
    

    this._container = container;
    
    this._addHandlers(container);
    this._updateGroup();
  },
  
  add: function(el){
    this._children.push( el );
    this._updateGroup();
    

    var bb = this._getBoundingBox();
    var padding = bb.top - $(this._container).position().top;    
    this._container.animate({
      width: bb.width + padding*2,
      height: bb.height + padding*2,
    },100);
    
  },
  
  remove: function(el){
    $(el).data("toRemove", true);
    this._children = this._children.filter(function(child){
      if( $(child).data("toRemove") == true ){
        $(child).data("group", null);
        return false;
      }
      else return true;
    })
    $(el).data("toRemove", false);
    
    if( this._children.length == 0 ){
      this._container.fadeOut(function() $(this).remove());
    }
    
  },
  
  _updateGroup: function(){
    var self = this;
    this._children.forEach(function(el){
      $(el).data("group", self);
    });    
  },
  
  _addHandlers: function(container){
    var self = this;
    
    $(container).draggable({
      start: function(){
        $(container).data("origPosition", $(container).position());
        self._children.forEach(function(el){
          $(el).data("origPosition", $(el).position());
        });
      },
      drag: function(e, ui){
        var origPos = $(container).data("origPosition");
        dX = ui.offset.left - origPos.left;
        dY = ui.offset.top - origPos.top;
        $(self._children).each(function(){
          $(this).css({
            left: $(this).data("origPosition").left + dX,
            top:  $(this).data("origPosition").top + dY
          })
        })
      }
    }).droppable({
      out: function(){
        $dragged.data("group").remove($dragged);
      }
    });
    
    }
}

var zIndex = 100;
var $dragged = null;
var timeout = null;

window.Groups = {
  dragOptions: {
    start:function(){
      $dragged = $(this);
      $dragged.data('isDragging', true);
    },
    stop: function(){
      $dragged.data('isDragging', false);
      $(this).css({zIndex: zIndex});
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
          top: target.position().top+10,
          left: target.position().left+10,      
        }, 100);
        
        setTimeout( function(){
          var group = $(target).data("group");
          if( group == null ){
            var group = new Group();
            group.create( [target, dragged] );            
          } else {
            group.add( dragged )
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
  }  
};

$(".tab").data('isDragging', false)
  .draggable(window.Groups.dragOptions)
  .droppable(window.Groups.dropOptions);



})();