











window.undefined = window.undefined;











var jQuery = function(a,c) {
	
	if ( window == this )
		return new jQuery(a,c);
	
	return this.init(a,c);
};


if ( typeof $ != "undefined" )
	jQuery._$ = $;
	

var $ = jQuery;





































 

































































jQuery.fn = jQuery.prototype = {
	








	init: function(a,c) {
		
		a = a || document;

		
		
		if ( jQuery.isFunction(a) )
			return new jQuery(document)[ jQuery.fn.ready ? "ready" : "load" ]( a );

		
		if ( typeof a  == "string" ) {
			
			var m = /^[^<]*(<(.|\s)+>)[^>]*$/.exec(a);
			if ( m )
				a = jQuery.clean( [ m[1] ] );

			
			else
				return new jQuery( c ).find( a );
		}

		return this.setArray(
			
			a.constructor == Array && a ||

			
			
			(a.jquery || a.length && a != window && !a.nodeType && a[0] != undefined && a[0].nodeType) && jQuery.makeArray( a ) ||

			
			[ a ] );
	},
	
	








	jquery: "@VERSION",

	












	











	size: function() {
		return this.length;
	},
	
	length: 0,

	
















	














	get: function( num ) {
		return num == undefined ?

			
			jQuery.makeArray( this ) :

			
			this[num];
	},
	
	












	pushStack: function( a ) {
		var ret = jQuery(a);
		ret.prevObject = this;
		return ret;
	},
	
	













	setArray: function( a ) {
		this.length = 0;
		[].push.apply( this, a );
		return this;
	},

	





















	each: function( fn, args ) {
		return jQuery.each( this, fn, args );
	},

	
























	index: function( obj ) {
		var pos = -1;
		this.each(function(i){
			if ( this == obj ) pos = i;
		});
		return pos;
	},

	


















	
















	

















	 
	























	attr: function( key, value, type ) {
		var obj = key;
		
		
		if ( key.constructor == String )
			if ( value == undefined )
				return this.length && jQuery[ type || "attr" ]( this[0], key ) || undefined;
			else {
				obj = {};
				obj[ key ] = value;
			}
		
		
		return this.each(function(index){
			
			for ( var prop in obj )
				jQuery.attr(
					type ? this.style : this,
					prop, jQuery.prop(this, obj[prop], type, index, prop)
				);
		});
	},

	




















	
















	



















	css: function( key, value ) {
		return this.attr( key, value, "curCSS" );
	},

	














	




















	text: function(e) {
		if ( typeof e == "string" )
			return this.empty().append( document.createTextNode( e ) );

		var t = "";
		jQuery.each( e || this, function(){
			jQuery.each( this.childNodes, function(){
				if ( this.nodeType != 8 )
					t += this.nodeType != 1 ?
						this.nodeValue : jQuery.fn.text([ this ]);
			});
		});
		return t;
	},

	























	





















	wrap: function() {
		
		var a, args = arguments;

		
		return this.each(function(){
			if ( !a )
				a = jQuery.clean(args, this.ownerDocument);

			
			var b = a[0].cloneNode(true);

			
			this.parentNode.insertBefore( b, this );

			
			while ( b.firstChild )
				b = b.firstChild;

			
			b.appendChild( this );
		});
	},

	




























	append: function() {
		return this.domManip(arguments, true, 1, function(a){
			this.appendChild( a );
		});
	},

	




























	prepend: function() {
		return this.domManip(arguments, true, -1, function(a){
			this.insertBefore( a, this.firstChild );
		});
	},
	
	

























	before: function() {
		return this.domManip(arguments, false, 1, function(a){
			this.parentNode.insertBefore( a, this );
		});
	},

	

























	after: function() {
		return this.domManip(arguments, false, -1, function(a){
			this.parentNode.insertBefore( a, this.nextSibling );
		});
	},

	





















	end: function() {
		return this.prevObject || jQuery([]);
	},

	



















	find: function(t) {
		return this.pushStack( jQuery.unique( jQuery.map( this, function(a){
			return jQuery.find(t,a);
		}) ), t );
	},

	















	clone: function(deep) {
		
		var $this = this.add(this.find("*"));
		$this.each(function() {
			this._$events = {};
			for (var type in this.$events)
				this._$events[type] = jQuery.extend({},this.$events[type]);
		}).unbind();

		
		var r = this.pushStack( jQuery.map( this, function(a){
			return a.cloneNode( deep != undefined ? deep : true );
		}) );

		
		$this.each(function() {
			var events = this._$events;
			for (var type in events)
				for (var handler in events[type])
					jQuery.event.add(this, type, events[type][handler], events[type][handler].data);
			this._$events = null;
		});

		
		return r;
	},

	





















	 
	
















	filter: function(t) {
		return this.pushStack(
			jQuery.isFunction( t ) &&
			jQuery.grep(this, function(el, index){
				return t.apply(el, [index])
			}) ||

			jQuery.multiFilter(t,this) );
	},

	














	















	

















	not: function(t) {
		return this.pushStack(
			t.constructor == String &&
			jQuery.multiFilter(t, this, true) ||

			jQuery.grep(this, function(a) {
				return ( t.constructor == Array || t.jquery )
					? jQuery.inArray( a, t ) < 0
					: a != t;
			})
		);
	},

	
















	 
	













	















	add: function(t) {
		return this.pushStack( jQuery.merge(
			this.get(),
			t.constructor == String ?
				jQuery(t).get() :
				t.length != undefined && (!t.nodeName || t.nodeName == "FORM") ?
					t : [t] )
		);
	},

	























	is: function(expr) {
		return expr ? jQuery.multiFilter(expr,this).length > 0 : false;
	},
	
	

















	
	











	val: function( val ) {
		return val == undefined ?
			( this.length ? this[0].value : null ) :
			this.attr( "value", val );
	},
	
	











	
	












	html: function( val ) {
		return val == undefined ?
			( this.length ? this[0].innerHTML : null ) :
			this.empty().append( val );
	},
	
	









	domManip: function(args, table, dir, fn){
		var clone = this.length > 1, a; 

		return this.each(function(){
			if ( !a ) {
				a = jQuery.clean(args, this.ownerDocument);
				if ( dir < 0 )
					a.reverse();
			}

			var obj = this;

			if ( table && jQuery.nodeName(this, "table") && jQuery.nodeName(a[0], "tr") )
				obj = this.getElementsByTagName("tbody")[0] || this.appendChild(document.createElement("tbody"));

			jQuery.each( a, function(){
				fn.apply( obj, [ clone ? this.cloneNode(true) : this ] );
			});

		});
	}
};




















































jQuery.extend = jQuery.fn.extend = function() {
	
	var target = arguments[0], a = 1;

	
	if ( arguments.length == 1 ) {
		target = this;
		a = 0;
	}
	var prop;
	while ( (prop = arguments[a++]) != null )
		
		for ( var i in prop ) target[i] = prop[i];

	
	return target;
};

jQuery.extend({
	

































	noConflict: function() {
		if ( jQuery._$ )
			$ = jQuery._$;
		return jQuery;
	},

	
	
	isFunction: function( fn ) {
		return !!fn && typeof fn != "string" && !fn.nodeName && 
			fn.constructor != Array && /function/i.test( fn + "" );
	},
	
	
	isXMLDoc: function(elem) {
		return elem.tagName && elem.ownerDocument && !elem.ownerDocument.body;
	},

	nodeName: function( elem, name ) {
		return elem.nodeName && elem.nodeName.toUpperCase() == name.toUpperCase();
	},

	



























	
	each: function( obj, fn, args ) {
		if ( obj.length == undefined )
			for ( var i in obj )
				fn.apply( obj[i], args || [i, obj[i]] );
		else
			for ( var i = 0, ol = obj.length; i < ol; i++ )
				if ( fn.apply( obj[i], args || [i, obj[i]] ) === false ) break;
		return obj;
	},
	
	prop: function(elem, value, type, index, prop){
			
			if ( jQuery.isFunction( value ) )
				value = value.call( elem, [index] );
				
			
			var exclude = /z-?index|font-?weight|opacity|zoom|line-?height/i;

			
			return value && value.constructor == Number && type == "curCSS" && !exclude.test(prop) ?
				value + "px" :
				value;
	},

	className: {
		
		add: function( elem, c ){
			jQuery.each( c.split(/\s+/), function(i, cur){
				if ( !jQuery.className.has( elem.className, cur ) )
					elem.className += ( elem.className ? " " : "" ) + cur;
			});
		},

		
		remove: function( elem, c ){
			elem.className = c != undefined ?
				jQuery.grep( elem.className.split(/\s+/), function(cur){
					return !jQuery.className.has( c, cur );	
				}).join(" ") : "";
		},

		
		has: function( t, c ) {
			return jQuery.inArray( c, (t.className || t).toString().split(/\s+/) ) > -1;
		}
	},

	



	swap: function(e,o,f) {
		for ( var i in o ) {
			e.style["old"+i] = e.style[i];
			e.style[i] = o[i];
		}
		f.apply( e, [] );
		for ( var i in o )
			e.style[i] = e.style["old"+i];
	},

	css: function(e,p) {
		if ( p == "height" || p == "width" ) {
			var old = {}, oHeight, oWidth, d = ["Top","Bottom","Right","Left"];

			jQuery.each( d, function(){
				old["padding" + this] = 0;
				old["border" + this + "Width"] = 0;
			});

			jQuery.swap( e, old, function() {
				if ( jQuery(e).is(':visible') ) {
					oHeight = e.offsetHeight;
					oWidth = e.offsetWidth;
				} else {
					e = jQuery(e.cloneNode(true))
						.find(":radio").removeAttr("checked").end()
						.css({
							visibility: "hidden", position: "absolute", display: "block", right: "0", left: "0"
						}).appendTo(e.parentNode)[0];

					var parPos = jQuery.css(e.parentNode,"position") || "static";
					if ( parPos == "static" )
						e.parentNode.style.position = "relative";

					oHeight = e.clientHeight;
					oWidth = e.clientWidth;

					if ( parPos == "static" )
						e.parentNode.style.position = "static";

					e.parentNode.removeChild(e);
				}
			});

			return p == "height" ? oHeight : oWidth;
		}

		return jQuery.curCSS( e, p );
	},

	curCSS: function(elem, prop, force) {
		var ret;

		if (prop == "opacity" && jQuery.browser.msie) {
			ret = jQuery.attr(elem.style, "opacity");
			return ret == "" ? "1" : ret;
		}
		
		if (prop.match(/float/i))
			prop = jQuery.browser.msie ? "styleFloat" : "cssFloat";

		if (!force && elem.style[prop])
			ret = elem.style[prop];

		else if (document.defaultView && document.defaultView.getComputedStyle) {

			if (prop.match(/float/i))
				prop = "float";

			prop = prop.replace(/([A-Z])/g,"-$1").toLowerCase();
			var cur = document.defaultView.getComputedStyle(elem, null);

			if ( cur )
				ret = cur.getPropertyValue(prop);
			else if ( prop == "display" )
				ret = "none";
			else
				jQuery.swap(elem, { display: "block" }, function() {
				    var c = document.defaultView.getComputedStyle(this, "");
				    ret = c && c.getPropertyValue(prop) || "";
				});

		} else if (elem.currentStyle) {
			var newProp = prop.replace(/\-(\w)/g,function(m,c){return c.toUpperCase();});
			ret = elem.currentStyle[prop] || elem.currentStyle[newProp];
		}

		return ret;
	},
	
	clean: function(a, doc) {
		var r = [];
		doc = doc || document;

		jQuery.each( a, function(i,arg){
			if ( !arg ) return;

			if ( arg.constructor == Number )
				arg = arg.toString();
			
			
			if ( typeof arg == "string" ) {
				
				var s = jQuery.trim(arg).toLowerCase(), div = doc.createElement("div"), tb = [];

				var wrap =
					
					!s.indexOf("<opt") &&
					[1, "<select>", "</select>"] ||
					
					!s.indexOf("<leg") &&
					[1, "<fieldset>", "</fieldset>"] ||
					
					(!s.indexOf("<thead") || !s.indexOf("<tbody") || !s.indexOf("<tfoot") || !s.indexOf("<colg")) &&
					[1, "<table>", "</table>"] ||
					
					!s.indexOf("<tr") &&
					[2, "<table><tbody>", "</tbody></table>"] ||
					
				 	
					(!s.indexOf("<td") || !s.indexOf("<th")) &&
					[3, "<table><tbody><tr>", "</tr></tbody></table>"] ||
					
					!s.indexOf("<col") &&
					[2, "<table><colgroup>", "</colgroup></table>"] ||
					
					[0,"",""];

				
				div.innerHTML = wrap[1] + arg + wrap[2];
				
				
				while ( wrap[0]-- )
					div = div.firstChild;
				
				
				if ( jQuery.browser.msie ) {
					
					
					if ( !s.indexOf("<table") && s.indexOf("<tbody") < 0 ) 
						tb = div.firstChild && div.firstChild.childNodes;
						
					
					else if ( wrap[1] == "<table>" && s.indexOf("<tbody") < 0 )
						tb = div.childNodes;

					for ( var n = tb.length-1; n >= 0 ; --n )
						if ( jQuery.nodeName(tb[n], "tbody") && !tb[n].childNodes.length )
							tb[n].parentNode.removeChild(tb[n]);
					
				}
				
				arg = jQuery.makeArray( div.childNodes );
			}

			if ( 0 === arg.length && !jQuery(arg).is("form, select") )
				return;

			if ( arg[0] == undefined || jQuery.nodeName(arg, "form") || arg.options )
				r.push( arg );
			else
				r = jQuery.merge( r, arg );

		});

		return r;
	},
	
	attr: function(elem, name, value){
		var fix = jQuery.isXMLDoc(elem) ? {} : {
			"for": "htmlFor",
			"class": "className",
			"float": jQuery.browser.msie ? "styleFloat" : "cssFloat",
			cssFloat: jQuery.browser.msie ? "styleFloat" : "cssFloat",
			styleFloat: jQuery.browser.msie ? "styleFloat" : "cssFloat",
			innerHTML: "innerHTML",
			className: "className",
			value: "value",
			disabled: "disabled",
			checked: "checked",
			readonly: "readOnly",
			selected: "selected",
			maxlength: "maxLength"
		};
		
		
		if ( name == "opacity" && jQuery.browser.msie ) {
			if ( value != undefined ) {
				
				
				elem.zoom = 1; 

				
				elem.filter = (elem.filter || "").replace(/alpha\([^)]*\)/,"") +
					(parseFloat(value).toString() == "NaN" ? "" : "alpha(opacity=" + value * 100 + ")");
			}

			return elem.filter ? 
				(parseFloat( elem.filter.match(/opacity=([^)]*)/)[1] ) / 100).toString() : "";
		}
		
		
		if ( fix[name] ) {
			if ( value != undefined ) elem[fix[name]] = value;
			return elem[fix[name]];

		} else if ( value == undefined && jQuery.browser.msie && jQuery.nodeName(elem, "form") && (name == "action" || name == "method") )
			return elem.getAttributeNode(name).nodeValue;

		
		else if ( elem.tagName ) {
			if ( value != undefined ) elem.setAttribute( name, value );
			if ( jQuery.browser.msie && /href|src/.test(name) && !jQuery.isXMLDoc(elem) ) 
				return elem.getAttribute( name, 2 );
			return elem.getAttribute( name );

		
		} else {
			name = name.replace(/-([a-z])/ig,function(z,b){return b.toUpperCase();});
			if ( value != undefined ) elem[name] = value;
			return elem[name];
		}
	},
	
	










	trim: function(t){
		return t.replace(/^\s+|\s+$/g, "");
	},

	makeArray: function( a ) {
		var r = [];

		
		if ( typeof a != "array" )
			for ( var i = 0, al = a.length; i < al; i++ )
				r.push( a[i] );
		else
			r = a.slice( 0 );

		return r;
	},

	inArray: function( b, a ) {
		for ( var i = 0, al = a.length; i < al; i++ )
			if ( a[i] == b )
				return i;
		return -1;
	},

	












	merge: function(first, second) {
		
		
		for ( var i = 0; second[i]; i++ )
			first.push(second[i]);
		return first;
	},

	











	unique: function(first) {
		var r = [], num = jQuery.mergeNum++;

		for ( var i = 0, fl = first.length; i < fl; i++ )
			if ( num != first[i].mergeNum ) {
				first[i].mergeNum = num;
				r.push(first[i]);
			}

		return r;
	},

	mergeNum: 0,

	



















	grep: function(elems, fn, inv) {
		
		
		if ( typeof fn == "string" )
			fn = new Function("a","i","return " + fn);

		var result = [];

		
		
		for ( var i = 0, el = elems.length; i < el; i++ )
			if ( !inv && fn(elems[i],i) || inv && !fn(elems[i],i) )
				result.push( elems[i] );

		return result;
	},

	




































	map: function(elems, fn) {
		
		
		if ( typeof fn == "string" )
			fn = new Function("a","return " + fn);

		var result = [];

		
		
		for ( var i = 0, el = elems.length; i < el; i++ ) {
			var val = fn(elems[i],i);

			if ( val !== null && val != undefined ) {
				if ( val.constructor != Array ) val = [val];
				result = result.concat( val );
			}
		}

		return result;
	}
});
























 








new function() {
	var b = navigator.userAgent.toLowerCase();

	
	jQuery.browser = {
		version: b.match(/.+(?:rv|it|ra|ie)[\/: ]([\d.]+)/)[1],
		safari: /webkit/.test(b),
		opera: /opera/.test(b),
		msie: /msie/.test(b) && !/opera/.test(b),
		mozilla: /mozilla/.test(b) && !/(compatible|webkit)/.test(b)
	};

	
	jQuery.boxModel = !jQuery.browser.msie || document.compatMode == "CSS1Compat";
};










































































































































jQuery.each({
	parent: "a.parentNode",
	parents: "jQuery.parents(a)",
	next: "jQuery.nth(a,2,'nextSibling')",
	prev: "jQuery.nth(a,2,'previousSibling')",
	siblings: "jQuery.sibling(a.parentNode.firstChild,a)",
	children: "jQuery.sibling(a.firstChild)"
}, function(i,n){
	jQuery.fn[ i ] = function(a) {
		var ret = jQuery.map(this,n);
		if ( a && typeof a == "string" )
			ret = jQuery.multiFilter(a,ret);
		return this.pushStack( ret );
	};
});









































































jQuery.each({
	appendTo: "append",
	prependTo: "prepend",
	insertBefore: "before",
	insertAfter: "after"
}, function(i,n){
	jQuery.fn[ i ] = function(){
		var a = arguments;
		return this.each(function(){
			for ( var j = 0, al = a.length; j < al; j++ )
				jQuery(a[j])[n]( this );
		});
	};
});




































































































jQuery.each( {
	removeAttr: function( key ) {
		jQuery.attr( this, key, "" );
		this.removeAttribute( key );
	},
	addClass: function(c){
		jQuery.className.add(this,c);
	},
	removeClass: function(c){
		jQuery.className.remove(this,c);
	},
	toggleClass: function( c ){
		jQuery.className[ jQuery.className.has(this,c) ? "remove" : "add" ](this, c);
	},
	remove: function(a){
		if ( !a || jQuery.filter( a, [this] ).r.length )
			this.parentNode.removeChild( this );
	},
	empty: function() {
		while ( this.firstChild )
			this.removeChild( this.firstChild );
	}
}, function(i,n){
	jQuery.fn[ i ] = function() {
		return this.each( n, arguments );
	};
});


























































jQuery.each( [ "eq", "lt", "gt", "contains" ], function(i,n){
	jQuery.fn[ n ] = function(num,fn) {
		return this.filter( ":" + n + "(" + num + ")", fn );
	};
});






























 






























jQuery.each( [ "height", "width" ], function(i,n){
	jQuery.fn[ n ] = function(h) {
		return h == undefined ?
			( this.length ? jQuery.css( this[0], n ) : null ) :
			this.css( n, h.constructor == String ? h : h + "px" );
	};
});
