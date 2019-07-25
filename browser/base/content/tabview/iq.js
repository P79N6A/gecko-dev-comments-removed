


















































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




iQ.fn = iQ.prototype = {
  
  
	init: function( selector, context ) {
    return null; 
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
							selector = [ document.createElement( ret[1] ) ];


						} else {
							selector = [ doc.createElement( ret[1] ) ];
						}

					} else {

						ret = buildFragment( [ match[1] ], [ doc ] );
						selector = (ret.cacheable ? ret.fragment.cloneNode(true) : ret.fragment).childNodes;
					}
					
					return iQ.merge( this, selector );
					
				
				} else {
					elem = document.getElementById( match[2] );

					if ( elem ) {
						
						
						if ( elem.id !== match[2] ) {
							return rootiQ.find( selector );
						}

						
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

	
	length: 0
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
	}
});



rootiQ = iQ(document);



window.iQ = iQ;

})(window);
