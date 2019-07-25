
















































(function( window, undefined ) {

var iQ = function(selector, context) {
    
    return new iQ.fn.init( selector, context );
  },

  
  _iQ = window.iQ,

  
  document = window.document,

  
  rootiQ,

  
  
  quickExpr = /^[^<]*(<[\w\W]+>)[^>]*$|^#([\w-]+)$/,

  
  rsingleTag = /^<(\w+)\s*\/?>(?:<\/\1>)?$/,

  
  toString = Object.prototype.toString,
  hasOwnProperty = Object.prototype.hasOwnProperty,

  rclass = /[\n\t]/g,
  rspace = /\s+/;




iQ.fn = iQ.prototype = {
  
  
  
  
  
  
  init: function( selector, context ) {
    var match, elem, ret, doc;

    
    if ( !selector ) {
      return this;
    }

    
    if ( selector.nodeType ) {
      this.context = this[0] = selector;
      this.length = 1;
      return this;
    }
    
    
    if ( selector === "body" && !context ) {
      this.context = document;
      this[0] = document.body;
      this.selector = "body";
      this.length = 1;
      return this;
    }

    
    if ( typeof selector === "string" ) {
      
      match = quickExpr.exec( selector );

      
      if ( match && (match[1] || !context) ) {

        
        if ( match[1] ) {
          doc = (context ? context.ownerDocument || context : document);

          
          
          ret = rsingleTag.exec( selector );

          if ( ret ) {
            if ( iQ.isPlainObject( context ) ) {
              Utils.assert('does not support HTML creation with context', false);
            } else {
              selector = [ doc.createElement( ret[1] ) ];
            }

          } else {
              Utils.assert('does not support complex HTML creation', false);
          }
          
          return iQ.merge( this, selector );
          
        
        } else {
          elem = document.getElementById( match[2] );

          if ( elem ) {
            this.length = 1;
            this[0] = elem;
          }

          this.context = document;
          this.selector = selector;
          return this;
        }

      
      } else if ( !context && /^\w+$/.test( selector ) ) {
        this.selector = selector;
        this.context = document;
        selector = document.getElementsByTagName( selector );
        return iQ.merge( this, selector );

      
      } else if ( !context || context.iq ) {
        return (context || rootiQ).find( selector );

      
      
      } else {
        return iQ( context ).find( selector );
      }

    
    
    } else if ( iQ.isFunction( selector ) ) {
      Utils.log('iQ does not support ready functions');
      return null;
    }

    if (selector.selector !== undefined) {
      this.selector = selector.selector;
      this.context = selector.context;
    }

    
    var ret = this || [];
    if ( selector != null ) {
      
      
      
      if ( selector.length == null || typeof selector === "string" || iQ.isFunction(selector) || (typeof selector !== "function" && selector.setInterval) ) {
        Array.prototype.push.call( ret, selector );
      } else {
        iQ.merge( ret, selector );
      }
    }
    return ret;
    
  },
  
  
  selector: "",

  
  iq: "1.4.2",

  
  length: 0, 
  
  
  
  
  
  get: function( num ) {
    return num == null ?

      
      
      Array.prototype.slice.call( this, 0 ) :

      
      ( num < 0 ? this[ num + this.length ] : this[ num ] );
  },

  
  
  
  
  
  each: function( callback, args ) {
    return iQ.each( this, callback, args );
  },

  
  
  addClass: function( value ) {
    if ( iQ.isFunction(value) ) {
      Utils.assert('does not support function argument', false);
      return null;
    }

    if ( value && typeof value === "string" ) {
      for ( var i = 0, l = this.length; i < l; i++ ) {
        var elem = this[i];
        if ( elem.nodeType === 1 ) {
          (value || "").split( rspace ).forEach(function(className) {
            elem.classList.add(className);
          });
        }
      }
    }

    return this;
  },

  
  
  removeClass: function( value ) {
    if ( iQ.isFunction(value) ) {
      Utils.assert('does not support function argument', false);
      return null;
    }

    if ( (value && typeof value === "string") || value === undefined ) {
      for ( var i = 0, l = this.length; i < l; i++ ) {
        var elem = this[i];
        if ( elem.nodeType === 1 && elem.className ) {
          if ( value ) {
            (value || "").split(rspace).forEach(function(className) {
              elem.classList.remove(className);
            });
          } else {
            elem.className = "";
          }
        }
      }
    }

    return this;
  },

  
  
  hasClass: function( selector ) {
    for ( var i = 0, l = this.length; i < l; i++ ) {
      if ( this[i].classList.contains( selector ) ) {
        return true;
      }
    }
    return false;
  },

  
  
  find: function( selector ) {
    var ret = [], length = 0;

    for ( var i = 0, l = this.length; i < l; i++ ) {
      length = ret.length;
      try {
        iQ.merge(ret, this[i].querySelectorAll( selector ) );
      } catch(e) {
        Utils.log('iQ.find error (bad selector)', e);
      }

      if ( i > 0 ) {
        
        for ( var n = length; n < ret.length; n++ ) {
          for ( var r = 0; r < length; r++ ) {
            if ( ret[r] === ret[n] ) {
              ret.splice(n--, 1);
              break;
            }
          }
        }
      }
    }

    return iQ(ret);
  },

  
  
  remove: function(unused) {
    Utils.assert('does not accept a selector', unused === undefined);
    for ( var i = 0, elem; (elem = this[i]) != null; i++ ) {
      if ( elem.parentNode ) {
         elem.parentNode.removeChild( elem );
      }
    }
    
    return this;
  },

  
  
  empty: function() {
    for ( var i = 0, elem; (elem = this[i]) != null; i++ ) {
      while ( elem.firstChild ) {
        elem.removeChild( elem.firstChild );
      }
    }
    
    return this;
  },

  
  
  width: function(unused) {
    Utils.assert('does not yet support setting', unused === undefined);
    return parseInt(this.css('width')); 
  },

  
  
  height: function(unused) {
    Utils.assert('does not yet support setting', unused === undefined);
    return parseInt(this.css('height'));
  },

  
  
  position: function(unused) {
    Utils.assert('does not yet support setting', unused === undefined);
    return {
      left: parseInt(this.css('left')),
      top: parseInt(this.css('top'))
    };
  },
  
  
  
  bounds: function(unused) {
    Utils.assert('does not yet support setting', unused === undefined);
    var p = this.position();
    return new Rect(p.left, p.top, this.width(), this.height());
  },
  
  
  
  data: function(key, value) {
    var data = null;
    if (value === undefined) {
      Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
      data = this[0].iQData;
      return (data ? data[key] : null);
    }
    
    for ( var i = 0, elem; (elem = this[i]) != null; i++ ) {
      data = elem.iQData;

      if (!data)
        data = elem.iQData = {};
        
      data[key] = value;
    }
    
    return this;    
  },
  
  
  
  
  html: function(value) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    if (value === undefined)
      return this[0].innerHTML;
      
    this[0].innerHTML = value;
    return this;
  },  
  
  
  
  text: function(value) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    if (value === undefined) {
      return this[0].textContent;
    }
      
    return this.empty().append( (this[0] && this[0].ownerDocument || document).createTextNode(value));
  },  
  
  
  
  val: function(value) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    if (value === undefined) {
      return this[0].value;
    }
    
    this[0].value = value;  
    return this;
  },  
  
  
  
  appendTo: function(selector) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    iQ(selector).append(this);
    return this;
  },
  
  
  
  append: function(selector) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    var object = iQ(selector);
    Utils.assert('does not yet support multi-objects (or null objects)', object.length == 1);
    this[0].appendChild(object[0]);
    return this;
  },
  
  
  
  
  attr: function(key, value) {
    try {
      Utils.assert('string key', typeof key === 'string');
      if (value === undefined) {
        Utils.assert('retrieval does not support multi-objects (or null objects)', this.length == 1);      
        return this[0].getAttribute(key);
      }
      for ( var i = 0, elem; (elem = this[i]) != null; i++ ) {
        elem.setAttribute(key, value);
      }    
    } catch(e) {
      Utils.log(e);
    }
    
    return this;
  },

  
  
  css: function(a, b) {
    var properties = null;
    
    if (typeof a === 'string') {
      var key = a; 
      if (b === undefined) {
        Utils.assert('retrieval does not support multi-objects (or null objects)', this.length == 1);      

        var substitutions = {
          'MozTransform': '-moz-transform',
          'zIndex': 'z-index'
        };

        return window.getComputedStyle(this[0], null).getPropertyValue(substitutions[key] || key);  
      }
      properties = {};
      properties[key] = b;
    } else {
      properties = a;
    }

    var pixels = {
      'left': true,
      'top': true,
      'right': true,
      'bottom': true,
      'width': true,
      'height': true
    };
    
    for ( var i = 0, elem; (elem = this[i]) != null; i++ ) {
      iQ.each(properties, function(key, value) {
        if (pixels[key] && typeof(value) != 'string') 
          value += 'px';
        
        if (key.indexOf('-') != -1)
          elem.style.setProperty(key, value, '');
        else
          elem.style[key] = value;
      });
    }
    
    return this; 
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  animate: function(css, options) {
    try {
      Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);

      if (!options)
        options = {};
      
      var easings = {
        tabcandyBounce: 'cubic-bezier(0.0, 0.63, .6, 1.29)', 
        easeInQuad: 'ease-in', 
        fast: 'cubic-bezier(0.7,0,1,1)'
      };
      
      var duration = (options.duration || 400);
      var easing = (easings[options.easing] || 'ease');

      
      
      
      var rupper = /([A-Z])/g;    
      this.each(function(){
        var cStyle = window.getComputedStyle(this, null);      
        for (var prop in css){
          prop = prop.replace( rupper, "-$1" ).toLowerCase();
          iQ(this).css(prop, cStyle.getPropertyValue(prop));
        }    
      });


      this.css({
        '-moz-transition-property': 'all', 
        '-moz-transition-duration': (duration / 1000) + 's',  
        '-moz-transition-timing-function': easing
      });

      this.css(css);
      
      var self = this;
      iQ.timeout(function() {
        self.css({
          '-moz-transition-property': 'none',  
          '-moz-transition-duration': '',  
          '-moz-transition-timing-function': ''
        });

        if (iQ.isFunction(options.complete))
          options.complete.apply(self);
      }, duration);
    } catch(e) {
      Utils.log(e);
    }
    
    return this;
  },
    
  
  
  fadeOut: function(callback) {
    try {
      Utils.assert('does not yet support duration', iQ.isFunction(callback) || callback === undefined);
      this.animate({
        opacity: 0
      }, {
        duration: 400,
        complete: function() {
          iQ(this).css({display: 'none'});
          if (iQ.isFunction(callback))
            callback.apply(this);
        }
      });  
    } catch(e) {
      Utils.log(e);
    }
    
    return this;
  },
    
  
  
  fadeIn: function() {
    try {
      this.css({display: ''});
      this.animate({
        opacity: 1
      }, {
        duration: 400
      });  
    } catch(e) {
      Utils.log(e);
    }
    
    return this;
  },
    
  
  
  hide: function() {
    try {
      this.css({display: 'none', opacity: 0});
    } catch(e) {
      Utils.log(e);
    }
    
    return this;
  },
    
  
  
  show: function() {
    try {
      this.css({display: '', opacity: 1});
    } catch(e) {
      Utils.log(e);
    }
    
    return this;
  },
    
  
  
  
  
  bind: function(type, func) {
    Utils.assert('does not support eventData argument', iQ.isFunction(func));

    var handler = function(event) {
      try {
        return func.apply(this, [event]);
      } catch(e) {
        Utils.log(e);
      }
    };

    for ( var i = 0, elem; (elem = this[i]) != null; i++ ) {
      if (!elem.iQEventData)
        elem.iQEventData = {};
        
      if (!elem.iQEventData[type])
        elem.iQEventData[type] = [];
        
      elem.iQEventData[type].push({
        original: func, 
        modified: handler
      });
      
      elem.addEventListener(type, handler, false);
    }
    
    return this; 
  },
  
  
  
  one: function(type, func) {
    Utils.assert('does not support eventData argument', iQ.isFunction(func));
    
    var handler = function(e) {
      iQ(this).unbind(type, handler);
      return func.apply(this, [e]);
    };
      
    return this.bind(type, handler);
  },
  
  
  
  unbind: function(type, func) {
    Utils.assert('Must provide a function', iQ.isFunction(func));
    
    for ( var i = 0, elem; (elem = this[i]) != null; i++ ) {
      var handler = func;
      if (elem.iQEventData && elem.iQEventData[type]) {
        for (var a = 0, count = elem.iQEventData[type].length; a < count; a++) {
          var pair = elem.iQEventData[type][a];
          if (pair.original == func) {
            handler = pair.modified; 
            elem.iQEventData[type].splice(a, 1);
            break;
          }
        }
      }
      
      elem.removeEventListener(type, handler, false);
    }
    
    return this; 
  }
};



iQ.fn.init.prototype = iQ.fn;



iQ.extend = iQ.fn.extend = function() {
  
  var target = arguments[0] || {}, i = 1, length = arguments.length, deep = false, options, name, src, copy;

  
  if ( typeof target === "boolean" ) {
    deep = target;
    target = arguments[1] || {};
    
    i = 2;
  }

  
  if ( typeof target !== "object" && !iQ.isFunction(target) ) {
    target = {};
  }

  
  if ( length === i ) {
    target = this;
    --i;
  }

  for ( ; i < length; i++ ) {
    
    if ( (options = arguments[ i ]) != null ) {
      
      for ( name in options ) {
        src = target[ name ];
        copy = options[ name ];

        
        if ( target === copy ) {
          continue;
        }

        
        if ( deep && copy && ( iQ.isPlainObject(copy) || iQ.isArray(copy) ) ) {
          var clone = src && ( iQ.isPlainObject(src) || iQ.isArray(src) ) ? src
            : iQ.isArray(copy) ? [] : {};

          
          target[ name ] = iQ.extend( deep, clone, copy );

        
        } else if ( copy !== undefined ) {
          target[ name ] = copy;
        }
      }
    }
  }

  
  return target;
};




iQ.extend({
  
  
  
  animationCount: 0,
  
  
  
  isAnimating: function() {
    return (this.animationCount != 0);
  },
  
  
  
  isFunction: function( obj ) {
    return toString.call(obj) === "[object Function]";
  },

  
  
  isArray: function( obj ) {
    return toString.call(obj) === "[object Array]";
  },

  
  
  isPlainObject: function( obj ) {
    
    
    
    if ( !obj || toString.call(obj) !== "[object Object]" || obj.nodeType || obj.setInterval ) {
      return false;
    }
    
    
    if ( obj.constructor
      && !hasOwnProperty.call(obj, "constructor")
      && !hasOwnProperty.call(obj.constructor.prototype, "isPrototypeOf") ) {
      return false;
    }
    
    
    
  
    var key;
    for ( key in obj ) {}
    
    return key === undefined || hasOwnProperty.call( obj, key );
  },

  
  
  isEmptyObject: function( obj ) {
    for ( var name in obj ) {
      return false;
    }
    return true;
  },

  
  
  
  each: function( object, callback ) {
    var name, i = 0,
      length = object.length,
      isObj = length === undefined || iQ.isFunction(object);

    if ( isObj ) {
      for ( name in object ) {
        if ( callback.call( object[ name ], name, object[ name ] ) === false ) {
          break;
        }
      }
    } else {
      for ( var value = object[0];
        i < length && callback.call( value, i, value ) !== false; value = object[++i] ) {}
    }

    return object;
  },
  
  
  
  merge: function( first, second ) {
    var i = first.length, j = 0;

    if ( typeof second.length === "number" ) {
      for ( var l = second.length; j < l; j++ ) {
        first[ i++ ] = second[ j ];
      }
    
    } else {
      while ( second[j] !== undefined ) {
        first[ i++ ] = second[ j++ ];
      }
    }

    first.length = i;

    return first;
  },

  
  
  
  timeout: function(func, delay) {
    setTimeout(function() { 
      try {
        func();
      } catch(e) {
        Utils.log(e);
      }
    }, delay);
  }
});



(function() {
  var events = [
    'keyup',
    'keydown',
    'mouseup',
    'mousedown',
    'mouseover',
    'mouseout',
    'mousemove',
    'click',
    'resize',
    'change',
    'blur',
    'focus'
  ];
  
  events.forEach(function(event) {
    iQ.fn[event] = function(func) {
      return this.bind(event, func);
    };
  });
})();



rootiQ = iQ(document);



window.iQ = iQ;

})(window);
