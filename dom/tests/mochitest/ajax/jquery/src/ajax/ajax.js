jQuery.fn.extend({

	














	loadIfModified: function( url, params, callback ) {
		this.load( url, params, callback, 1 );
	},

	























	load: function( url, params, callback, ifModified ) {
		if ( jQuery.isFunction( url ) )
			return this.bind("load", url);

		callback = callback || function(){};

		
		var type = "GET";

		
		if ( params )
			
			if ( jQuery.isFunction( params ) ) {
				
				callback = params;
				params = null;

			
			} else {
				params = jQuery.param( params );
				type = "POST";
			}

		var self = this;

		
		jQuery.ajax({
			url: url,
			type: type,
			data: params,
			ifModified: ifModified,
			complete: function(res, status){
				if ( status == "success" || !ifModified && status == "notmodified" )
					
					self.attr("innerHTML", res.responseText)
					  
					  .evalScripts()
					  
					  .each( callback, [res.responseText, status, res] );
				else
					callback.apply( self, [res.responseText, status, res] );
			}
		});
		return this;
	},

	


















	serialize: function() {
		return jQuery.param( this );
	},

	








	evalScripts: function() {
		return this.find("script").each(function(){
			if ( this.src )
				jQuery.getScript( this.src );
			else
				jQuery.globalEval( this.text || this.textContent || this.innerHTML || "" );
		}).end();
	}

});





















































































 
















jQuery.each( "ajaxStart,ajaxStop,ajaxComplete,ajaxError,ajaxSuccess,ajaxSend".split(","), function(i,o){
	jQuery.fn[o] = function(f){
		return this.bind(o, f);
	};
});

jQuery.extend({

	































	get: function( url, data, callback, type, ifModified ) {
		
		if ( jQuery.isFunction( data ) ) {
			callback = data;
			data = null;
		}
		
		return jQuery.ajax({
			type: "GET",
			url: url,
			data: data,
			success: callback,
			dataType: type,
			ifModified: ifModified
		});
	},

	

























	getIfModified: function( url, data, callback, type ) {
		return jQuery.get(url, data, callback, type, 1);
	},

	


















	getScript: function( url, callback ) {
		return jQuery.get(url, null, callback, "script");
	},

	




















	getJSON: function( url, data, callback ) {
		return jQuery.get(url, data, callback, "json");
	},

	
























	post: function( url, data, callback, type ) {
		if ( jQuery.isFunction( data ) ) {
			callback = data;
			data = {};
		}

		return jQuery.ajax({
			type: "POST",
			url: url,
			data: data,
			success: callback,
			dataType: type
		});
	},

	



















	ajaxTimeout: function( timeout ) {
		jQuery.ajaxSettings.timeout = timeout;
	},
	
	



















	ajaxSetup: function( settings ) {
		jQuery.extend( jQuery.ajaxSettings, settings );
	},

	ajaxSettings: {
		global: true,
		type: "GET",
		timeout: 0,
		contentType: "application/x-www-form-urlencoded",
		processData: true,
		async: true,
		data: null
	},
	
	
	lastModified: {},

	

























































































































	ajax: function( s ) {
		
		s = jQuery.extend({}, jQuery.ajaxSettings, s);

		
		if ( s.data ) {
			
			if (s.processData && typeof s.data != "string")
    			s.data = jQuery.param(s.data);
			
			if( s.type.toLowerCase() == "get" ) {
				
				s.url += ((s.url.indexOf("?") > -1) ? "&" : "?") + s.data;
				
				s.data = null;
			}
		}

		
		if ( s.global && ! jQuery.active++ )
			jQuery.event.trigger( "ajaxStart" );

		var requestDone = false;

		
		
		var xml = window.ActiveXObject ? new ActiveXObject("Microsoft.XMLHTTP") : new XMLHttpRequest();

		
		xml.open(s.type, s.url, s.async);

		
		if ( s.data )
			xml.setRequestHeader("Content-Type", s.contentType);

		
		if ( s.ifModified )
			xml.setRequestHeader("If-Modified-Since",
				jQuery.lastModified[s.url] || "Thu, 01 Jan 1970 00:00:00 GMT" );

		
		xml.setRequestHeader("X-Requested-With", "XMLHttpRequest");

		
		if( s.beforeSend )
			s.beforeSend(xml);
			
		if ( s.global )
		    jQuery.event.trigger("ajaxSend", [xml, s]);

		
		var onreadystatechange = function(isTimeout){
			
			if ( xml && (xml.readyState == 4 || isTimeout == "timeout") ) {
				requestDone = true;
				
				
				if (ival) {
					clearInterval(ival);
					ival = null;
				}
				
				var status;
				try {
					status = jQuery.httpSuccess( xml ) && isTimeout != "timeout" ?
						s.ifModified && jQuery.httpNotModified( xml, s.url ) ? "notmodified" : "success" : "error";
					
					if ( status != "error" ) {
						
						var modRes;
						try {
							modRes = xml.getResponseHeader("Last-Modified");
						} catch(e) {} 
	
						if ( s.ifModified && modRes )
							jQuery.lastModified[s.url] = modRes;
	
						
						var data = jQuery.httpData( xml, s.dataType );
	
						
						if ( s.success )
							s.success( data, status );
	
						
						if( s.global )
							jQuery.event.trigger( "ajaxSuccess", [xml, s] );
					} else
						jQuery.handleError(s, xml, status);
				} catch(e) {
					status = "error";
					jQuery.handleError(s, xml, status, e);
				}

				
				if( s.global )
					jQuery.event.trigger( "ajaxComplete", [xml, s] );

				
				if ( s.global && ! --jQuery.active )
					jQuery.event.trigger( "ajaxStop" );

				
				if ( s.complete )
					s.complete(xml, status);

				
				if(s.async)
					xml = null;
			}
		};
		
		
		var ival = setInterval(onreadystatechange, 13); 

		
		if ( s.timeout > 0 )
			setTimeout(function(){
				
				if ( xml ) {
					
					xml.abort();

					if( !requestDone )
						onreadystatechange( "timeout" );
				}
			}, s.timeout);
			
		
		try {
			xml.send(s.data);
		} catch(e) {
			jQuery.handleError(s, xml, null, e);
		}
		
		
		if ( !s.async )
			onreadystatechange();
		
		
		return xml;
	},

	handleError: function( s, xml, status, e ) {
		
		if ( s.error ) s.error( xml, status, e );

		
		if ( s.global )
			jQuery.event.trigger( "ajaxError", [xml, s, e] );
	},

	
	active: 0,

	
	httpSuccess: function( r ) {
		try {
			return !r.status && location.protocol == "file:" ||
				( r.status >= 200 && r.status < 300 ) || r.status == 304 ||
				jQuery.browser.safari && r.status == undefined;
		} catch(e){}
		return false;
	},

	
	httpNotModified: function( xml, url ) {
		try {
			var xmlRes = xml.getResponseHeader("Last-Modified");

			
			return xml.status == 304 || xmlRes == jQuery.lastModified[url] ||
				jQuery.browser.safari && xml.status == undefined;
		} catch(e){}
		return false;
	},

	





	httpData: function( r, type ) {
		var ct = r.getResponseHeader("content-type");
		var data = !type && ct && ct.indexOf("xml") >= 0;
		data = type == "xml" || data ? r.responseXML : r.responseText;

		
		if ( type == "script" )
			jQuery.globalEval( data );

		
		if ( type == "json" )
			data = eval("(" + data + ")");

		
		if ( type == "html" )
			jQuery("<div>").html(data).evalScripts();

		return data;
	},

	
	
	param: function( a ) {
		var s = [];

		
		
		if ( a.constructor == Array || a.jquery )
			
			jQuery.each( a, function(){
				s.push( encodeURIComponent(this.name) + "=" + encodeURIComponent( this.value ) );
			});

		
		else
			
			for ( var j in a )
				
				if ( a[j] && a[j].constructor == Array )
					jQuery.each( a[j], function(){
						s.push( encodeURIComponent(j) + "=" + encodeURIComponent( this ) );
					});
				else
					s.push( encodeURIComponent(j) + "=" + encodeURIComponent( a[j] ) );

		
		return s.join("&");
	},
	
	
	
	globalEval: function( data ) {
		if ( window.execScript )
			window.execScript( data );
		else if ( jQuery.browser.safari )
			
			window.setTimeout( data, 0 );
		else
			eval.call( window, data );
	}

});
