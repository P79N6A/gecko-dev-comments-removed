


















































(function( window, undefined ) {

var iQ = function(selector, context) {
		
		return new iQ.fn.init( selector, context );
	},

	
	_iQ = window.iQ,

	
	document = window.document,

	
	rootiQ,

	
	
	quickExpr = /^[^<]*(<[\w\W]+>)[^>]*$|^#([\w-]+)$/,

	
	isSimple = /^.[^:#\[\.,]*$/,

	
	rnotwhite = /\S/,

	
	rtrim = /^(\s|\u00A0)+|(\s|\u00A0)+$/g,

	
	rsingleTag = /^<(\w+)\s*\/?>(?:<\/\1>)?$/,

	
	toString = Object.prototype.toString,
	hasOwnProperty = Object.prototype.hasOwnProperty,
	push = Array.prototype.push,
	slice = Array.prototype.slice,
	indexOf = Array.prototype.indexOf;

var rclass = /[\n\t]/g,
	rspace = /\s+/,
	rreturn = /\r/g,
	rspecialurl = /href|src|style/,
	rtype = /(button|input)/i,
	rfocusable = /(button|input|object|select|textarea)/i,
	rclickable = /^(a|area)$/i,
	rradiocheck = /radio|checkbox/;




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
							selector = [ document.createElement( ret[1] ) ];


						} else {
							selector = [ doc.createElement( ret[1] ) ];
						}

					} else {
							Utils.assert('does not support complex HTML creation', false);

						ret = buildFragment( [ match[1] ], [ doc ] );
						selector = (ret.cacheable ? ret.fragment.cloneNode(true) : ret.fragment).childNodes;
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

		return iQ.makeArray( selector, this );
	},
	
	
	selector: "",

	
	iq: "1.4.2",

	
	length: 0, 
	
  
  
	toArray: function() {
		return slice.call( this, 0 );
	},

  
  
	
	
	get: function( num ) {
		return num == null ?

			
			this.toArray() :

			
			( num < 0 ? this.slice(num)[ 0 ] : this[ num ] );
	},

  
  
	
	
	pushStack: function( elems, name, selector ) {
		
		var ret = iQ();

		if ( iQ.isArray( elems ) ) {
			push.apply( ret, elems );
		
		} else {
			iQ.merge( ret, elems );
		}

		
		ret.prevObject = this;

		ret.context = this.context;

		if ( name === "find" ) {
			ret.selector = this.selector + (this.selector ? " " : "") + selector;
		} else if ( name ) {
			ret.selector = this.selector + "." + name + "(" + selector + ")";
		}

		
		return ret;
	},

  
  
	
	
	
	each: function( callback, args ) {
		return iQ.each( this, callback, args );
	},
	
  
  
	slice: function() {
		return this.pushStack( slice.apply( this, arguments ),
			"slice", slice.call(arguments).join(",") );
	},

  
  
	addClass: function( value ) {
		if ( iQ.isFunction(value) ) {
		  Utils.assert('does not support function argument', false);
		  return null;
		}

		if ( value && typeof value === "string" ) {
			var classNames = (value || "").split( rspace );

			for ( var i = 0, l = this.length; i < l; i++ ) {
				var elem = this[i];

				if ( elem.nodeType === 1 ) {
					if ( !elem.className ) {
						elem.className = value;

					} else {
						var className = " " + elem.className + " ", setClass = elem.className;
						for ( var c = 0, cl = classNames.length; c < cl; c++ ) {
							if ( className.indexOf( " " + classNames[c] + " " ) < 0 ) {
								setClass += " " + classNames[c];
							}
						}
						elem.className = iQ.trim( setClass );
					}
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
			var classNames = (value || "").split(rspace);

			for ( var i = 0, l = this.length; i < l; i++ ) {
				var elem = this[i];

				if ( elem.nodeType === 1 && elem.className ) {
					if ( value ) {
						var className = (" " + elem.className + " ").replace(rclass, " ");
						for ( var c = 0, cl = classNames.length; c < cl; c++ ) {
							className = className.replace(" " + classNames[c] + " ", " ");
						}
						elem.className = iQ.trim( className );

					} else {
						elem.className = "";
					}
				}
			}
		}

		return this;
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
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    return this[0].clientWidth;
  },

  
  
	height: function(unused) {
    Utils.assert('does not yet support setting', unused === undefined);
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    return this[0].clientHeight;
  },

  
  
  bounds: function(unused) {
    Utils.assert('does not yet support setting', unused === undefined);
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    var el = this[0];
    return new Rect(
      parseInt(el.style.left) || el.offsetLeft, 
      parseInt(el.style.top) || el.offsetTop, 
      el.clientWidth,
      el.clientHeight
    );
  },
  
  
  
  data: function(key, value) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    var data = this[0].iQData;
    if(value === undefined)
      return (data ? data[key] : null);
    
    if(!data)
      data = this[0].iQData = {};
      
    data[key] = value;
    return this;    
  },
  
  
  
  
  html: function(value) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    if(value === undefined)
      return this[0].innerHTML;
      
    this[0].innerHTML = value;
    return this;
  },  
  
  
  
  text: function(value) {
    Utils.assert('does not yet support multi-objects (or null objects)', this.length == 1);
    if(value === undefined) {
      return this[0].textContent;
    }
      
		return this.empty().append( (this[0] && this[0].ownerDocument || document).createTextNode(value));
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

	
  
	
	each: function( object, callback, args ) {
		var name, i = 0,
			length = object.length,
			isObj = length === undefined || iQ.isFunction(object);

		if ( args ) {
			if ( isObj ) {
				for ( name in object ) {
					if ( callback.apply( object[ name ], args ) === false ) {
						break;
					}
				}
			} else {
				for ( ; i < length; ) {
					if ( callback.apply( object[ i++ ], args ) === false ) {
						break;
					}
				}
			}

		
		} else {
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
		}

		return object;
	},
	
  
  
	trim: function( text ) {
		return (text || "").replace( rtrim, "" );
	},

  
  
	
	makeArray: function( array, results ) {
		var ret = results || [];

		if ( array != null ) {
			
			
			
			if ( array.length == null || typeof array === "string" || iQ.isFunction(array) || (typeof array !== "function" && array.setInterval) ) {
				push.call( ret, array );
			} else {
				iQ.merge( ret, array );
			}
		}

		return ret;
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

  
  
	grep: function( elems, callback, inv ) {
		var ret = [];

		
		
		for ( var i = 0, length = elems.length; i < length; i++ ) {
			if ( !inv !== !callback( elems[ i ], i ) ) {
				ret.push( elems[ i ] );
			}
		}

		return ret;
	}	
});



rootiQ = iQ(document);



window.iQ = iQ;

})(window);
