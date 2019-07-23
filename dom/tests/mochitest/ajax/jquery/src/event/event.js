




jQuery.event = {

	
	
	add: function(element, type, handler, data) {
		
		
		if ( jQuery.browser.msie && element.setInterval != undefined )
			element = window;
		
		
		if ( !handler.guid )
			handler.guid = this.guid++;
			
		
		if( data != undefined ) { 
        	
			var fn = handler; 

			
			handler = function() { 
				
				return fn.apply(this, arguments); 
			};

			
			handler.data = data;

			
			handler.guid = fn.guid;
		}

		
		if (!element.$events)
			element.$events = {};
		
		if (!element.$handle)
			element.$handle = function() {
				
				var val;

				
				
				if ( typeof jQuery == "undefined" || jQuery.event.triggered )
				  return val;
				
				val = jQuery.event.handle.apply(element, arguments);
				
				return val;
			};

		
		var handlers = element.$events[type];

		
		if (!handlers) {
			handlers = element.$events[type] = {};	
			
			
			if (element.addEventListener)
				element.addEventListener(type, element.$handle, false);
			else if (element.attachEvent)
				element.attachEvent("on" + type, element.$handle);
		}

		
		handlers[handler.guid] = handler;

		
		if (!this.global[type])
			this.global[type] = [];
		
		if (jQuery.inArray(element, this.global[type]) == -1)
			this.global[type].push( element );
	},

	guid: 1,
	global: {},

	
	remove: function(element, type, handler) {
		var events = element.$events, ret, index;

		if ( events ) {
			
			if ( type && type.type ) {
				handler = type.handler;
				type = type.type;
			}
			
			if ( !type ) {
				for ( type in events )
					this.remove( element, type );

			} else if ( events[type] ) {
				
				if ( handler )
					delete events[type][handler.guid];
				
				
				else
					for ( handler in element.$events[type] )
						delete events[type][handler];

				
				for ( ret in events[type] ) break;
				if ( !ret ) {
					if (element.removeEventListener)
						element.removeEventListener(type, element.$handle, false);
					else if (element.detachEvent)
						element.detachEvent("on" + type, element.$handle);
					ret = null;
					delete events[type];
					
					
					while ( this.global[type] && ( (index = jQuery.inArray(element, this.global[type])) >= 0 ) )
						delete this.global[type][index];
				}
			}

			
			for ( ret in events ) break;
			if ( !ret )
				element.$handle = element.$events = null;
		}
	},

	trigger: function(type, data, element) {
		
		data = jQuery.makeArray(data || []);

		
		if ( !element )
			jQuery.each( this.global[type] || [], function(){
				jQuery.event.trigger( type, data, this );
			});

		
		else {
			var val, ret, fn = jQuery.isFunction( element[ type ] || null );
			
			
			data.unshift( this.fix({ type: type, target: element }) );

			
			if ( jQuery.isFunction(element.$handle) && (val = element.$handle.apply( element, data )) !== false )
				this.triggered = true;

			if ( fn && val !== false && !jQuery.nodeName(element, 'a') )
				element[ type ]();

			this.triggered = false;
		}
	},

	handle: function(event) {
		
		var val;

		
		event = jQuery.event.fix( event || window.event || {} ); 

		var c = this.$events && this.$events[event.type], args = [].slice.call( arguments, 1 );
		args.unshift( event );

		for ( var j in c ) {
			
			
			args[0].handler = c[j];
			args[0].data = c[j].data;

			if ( c[j].apply( this, args ) === false ) {
				event.preventDefault();
				event.stopPropagation();
				val = false;
			}
		}

		
		if (jQuery.browser.msie)
			event.target = event.preventDefault = event.stopPropagation =
				event.handler = event.data = null;

		return val;
	},

	fix: function(event) {
		
		
		var originalEvent = event;
		event = jQuery.extend({}, originalEvent);
		
		
		
		event.preventDefault = function() {
			
			if (originalEvent.preventDefault)
				return originalEvent.preventDefault();
			
			originalEvent.returnValue = false;
		};
		event.stopPropagation = function() {
			
			if (originalEvent.stopPropagation)
				return originalEvent.stopPropagation();
			
			originalEvent.cancelBubble = true;
		};
		
		
		if ( !event.target && event.srcElement )
			event.target = event.srcElement;
				
		
		if (jQuery.browser.safari && event.target.nodeType == 3)
			event.target = originalEvent.target.parentNode;

		
		if ( !event.relatedTarget && event.fromElement )
			event.relatedTarget = event.fromElement == event.target ? event.toElement : event.fromElement;

		
		if ( event.pageX == null && event.clientX != null ) {
			var e = document.documentElement || document.body;
			event.pageX = event.clientX + e.scrollLeft;
			event.pageY = event.clientY + e.scrollTop;
		}
			
		
		if ( !event.which && (event.charCode || event.keyCode) )
			event.which = event.charCode || event.keyCode;
		
		
		if ( !event.metaKey && event.ctrlKey )
			event.metaKey = event.ctrlKey;

		
		
		if ( !event.which && event.button )
			event.which = (event.button & 1 ? 1 : ( event.button & 2 ? 3 : ( event.button & 4 ? 2 : 0 ) ));
			
		return event;
	}
};

jQuery.fn.extend({

	
















































	bind: function( type, data, fn ) {
		return type == "unload" ? this.one(type, data, fn) : this.each(function(){
			jQuery.event.add( this, type, fn || data, fn && data );
		});
	},
	
	

























	one: function( type, data, fn ) {
		return this.each(function(){
			jQuery.event.add( this, type, function(event) {
				jQuery(this).unbind(event);
				return (fn || data).apply( this, arguments);
			}, fn && data);
		});
	},

	




























	unbind: function( type, fn ) {
		return this.each(function(){
			jQuery.event.remove( this, type, fn );
		});
	},

	































	trigger: function( type, data ) {
		return this.each(function(){
			jQuery.event.trigger( type, data, this );
		});
	},

	



















	toggle: function() {
		
		var a = arguments;

		return this.click(function(e) {
			
			this.lastToggle = 0 == this.lastToggle ? 1 : 0;
			
			
			e.preventDefault();
			
			
			return a[this.lastToggle].apply( this, [e] ) || false;
		});
	},
	
	
























	hover: function(f,g) {
		
		
		function handleHover(e) {
			
			var p = e.relatedTarget;
	
			
			while ( p && p != this ) try { p = p.parentNode } catch(e) { p = this; };
			
			
			if ( p == this ) return false;
			
			
			return (e.type == "mouseover" ? f : g).apply(this, [e]);
		}
		
		
		return this.mouseover(handleHover).mouseout(handleHover);
	},
	
	




































	ready: function(f) {
		
		if ( jQuery.isReady )
			
			f.apply( document, [jQuery] );
			
		
		else {
			
			jQuery.readyList.push( function() { return f.apply(this, [jQuery]) } );
		}
	
		return this;
	}
});

jQuery.extend({
	


	isReady: false,
	readyList: [],
	
	
	ready: function() {
		
		if ( !jQuery.isReady ) {
			
			jQuery.isReady = true;
			
			
			if ( jQuery.readyList ) {
				
				jQuery.each( jQuery.readyList, function(){
					this.apply( document );
				});
				
				
				jQuery.readyList = null;
			}
			
			if ( jQuery.browser.mozilla || jQuery.browser.opera )
				document.removeEventListener( "DOMContentLoaded", jQuery.ready, false );
			
			
			if( !window.frames.length ) 
				jQuery(window).load(function(){ jQuery("#__ie_init").remove(); });
		}
	}
});

new function(){

	












	














	
















	












	















	












	












	












	












	












	

















	












	












	














	












	












	












	












	












	












	












	












	












	











	 
	











	jQuery.each( ("blur,focus,load,resize,scroll,unload,click,dblclick," +
		"mousedown,mouseup,mousemove,mouseover,mouseout,change,select," + 
		"submit,keydown,keypress,keyup,error").split(","), function(i,o){
		
		
		jQuery.fn[o] = function(f){
			return f ? this.bind(o, f) : this.trigger(o);
		};
			
	});
	
	
	if ( jQuery.browser.mozilla || jQuery.browser.opera )
		
		document.addEventListener( "DOMContentLoaded", jQuery.ready, false );
	
	
	
	else if ( jQuery.browser.msie ) {
	
		
		document.write("<scr" + "ipt id=__ie_init defer=true " + 
			"src=//:><\/script>");
	
		
		var script = document.getElementById("__ie_init");
		
		
		if ( script ) 
			script.onreadystatechange = function() {
				if ( this.readyState != "complete" ) return;
				jQuery.ready();
			};
	
		
		script = null;
	
	
	} else if ( jQuery.browser.safari )
		
		jQuery.safariTimer = setInterval(function(){
			
			if ( document.readyState == "loaded" || 
				document.readyState == "complete" ) {
	
				
				clearInterval( jQuery.safariTimer );
				jQuery.safariTimer = null;
	
				
				jQuery.ready();
			}
		}, 10); 

	
	jQuery.event.add( window, "load", jQuery.ready );
	
};


if (jQuery.browser.msie)
	jQuery(window).one("unload", function() {
		var global = jQuery.event.global;
		for ( var type in global ) {
			var els = global[type], i = els.length;
			if ( i && type != 'unload' )
				do
					els[i-1] && jQuery.event.remove(els[i-1], type);
				while (--i);
		}
	});
