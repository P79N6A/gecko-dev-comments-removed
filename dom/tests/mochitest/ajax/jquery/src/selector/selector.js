jQuery.extend({
	expr: {
		"": "m[2]=='*'||jQuery.nodeName(a,m[2])",
		"#": "a.getAttribute('id')==m[2]",
		":": {
			
			lt: "i<m[3]-0",
			gt: "i>m[3]-0",
			nth: "m[3]-0==i",
			eq: "m[3]-0==i",
			first: "i==0",
			last: "i==r.length-1",
			even: "i%2==0",
			odd: "i%2",

			
			"nth-child": "jQuery.nth(a.parentNode.firstChild,m[3],'nextSibling',a)==a",
			"first-child": "jQuery.nth(a.parentNode.firstChild,1,'nextSibling')==a",
			"last-child": "jQuery.nth(a.parentNode.lastChild,1,'previousSibling')==a",
			"only-child": "jQuery.sibling(a.parentNode.firstChild).length==1",

			
			parent: "a.firstChild",
			empty: "!a.firstChild",

			
			contains: "jQuery.fn.text.apply([a]).indexOf(m[3])>=0",

			
			visible: '"hidden"!=a.type&&jQuery.css(a,"display")!="none"&&jQuery.css(a,"visibility")!="hidden"',
			hidden: '"hidden"==a.type||jQuery.css(a,"display")=="none"||jQuery.css(a,"visibility")=="hidden"',

			
			enabled: "!a.disabled",
			disabled: "a.disabled",
			checked: "a.checked",
			selected: "a.selected||jQuery.attr(a,'selected')",

			
			text: "'text'==a.type",
			radio: "'radio'==a.type",
			checkbox: "'checkbox'==a.type",
			file: "'file'==a.type",
			password: "'password'==a.type",
			submit: "'submit'==a.type",
			image: "'image'==a.type",
			reset: "'reset'==a.type",
			button: '"button"==a.type||jQuery.nodeName(a,"button")',
			input: "/input|select|textarea|button/i.test(a.nodeName)"
		},
		".": "jQuery.className.has(a,m[2])",
		"@": {
			"=": "z==m[4]",
			"!=": "z!=m[4]",
			"^=": "z&&!z.indexOf(m[4])",
			"$=": "z&&z.substr(z.length - m[4].length,m[4].length)==m[4]",
			"*=": "z&&z.indexOf(m[4])>=0",
			"": "z",
			_resort: function(m){
				return ["", m[1], m[3], m[2], m[5]];
			},
			_prefix: "var z=a[m[3]];if(!z||/href|src/.test(m[3]))z=jQuery.attr(a,m[3]);"
		},
		"[": "jQuery.find(m[2],a).length"
	},
	
	
	parse: [
		
		/^\[ *(@)([\w-]+) *([!*$^=]*) *('?"?)(.*?)\4 *\]/,

		
		/^(\[)\s*(.*?(\[.*?\])?[^[]*?)\s*\]/,

		
		/^(:)([\w-]+)\("?'?(.*?(\(.*?\))?[^(]*?)"?'?\)/,

		
		new RegExp("^([:.#]*)(" + 
			( jQuery.chars = "(?:[\\w\u0128-\uFFFF*_-]|\\\\.)" ) + "+)")
	],

	token: [
		/^(\/?\.\.)/, "a.parentNode",
		/^(>|\/)/, "jQuery.sibling(a.firstChild)",
		/^(\+)/, "jQuery.nth(a,2,'nextSibling')",
		/^(~)/, function(a){
			var s = jQuery.sibling(a.parentNode.firstChild);
			return s.slice(jQuery.inArray(a,s) + 1);
		}
	],

	multiFilter: function( expr, elems, not ) {
		var old, cur = [];

		while ( expr && expr != old ) {
			old = expr;
			var f = jQuery.filter( expr, elems, not );
			expr = f.t.replace(/^\s*,\s*/, "" );
			cur = not ? elems = f.r : jQuery.merge( cur, f.r );
		}

		return cur;
	},

	





	find: function( t, context ) {
		
		if ( typeof t != "string" )
			return [ t ];

		
		if ( context && !context.nodeType )
			context = null;

		
		context = context || document;

		
		if ( !t.indexOf("//") ) {
			context = context.documentElement;
			t = t.substr(2,t.length);

		
		} else if ( !t.indexOf("/") && !context.ownerDocument ) {
			context = context.documentElement;
			t = t.substr(1,t.length);
			if ( t.indexOf("/") >= 1 )
				t = t.substr(t.indexOf("/"),t.length);
		}

		
		var ret = [context], done = [], last;

		
		
		while ( t && last != t ) {
			var r = [];
			last = t;

			t = jQuery.trim(t).replace( /^\/\//, "" );

			var foundToken = false;

			
			
			var re = new RegExp("^[/>]\\s*(" + jQuery.chars + "+)");
			var m = re.exec(t);

			if ( m ) {
				
				for ( var i = 0; ret[i]; i++ )
					for ( var c = ret[i].firstChild; c; c = c.nextSibling )
						if ( c.nodeType == 1 && ( m[1] == "*" || jQuery.nodeName(c, m[1]) ) )
							r.push( c );

				ret = r;
				t = t.replace( re, "" );
				if ( t.indexOf(" ") == 0 ) continue;
				foundToken = true;
			} else {
				
				for ( var i = 0, tl = jQuery.token.length; i < tl; i += 2 ) {
					
					
					var re = jQuery.token[i], fn = jQuery.token[i+1];
					var m = re.exec(t);

					
					if ( m ) {
						
						r = ret = jQuery.map( ret, jQuery.isFunction( fn ) ?
							fn : new Function( "a", "return " + fn ) );

						
						t = jQuery.trim( t.replace( re, "" ) );
						foundToken = true;
						break;
					}
				}
			}

			
			
			if ( t && !foundToken ) {
				
				if ( !t.indexOf(",") ) {
					
					if ( context == ret[0] ) ret.shift();

					
					done = jQuery.merge( done, ret );

					
					r = ret = [context];

					
					t = " " + t.substr(1,t.length);

				} else {
					
					var re2 = new RegExp("^(" + jQuery.chars + "+)(#)(" + jQuery.chars + "+)");
					var m = re2.exec(t);
					
					
					if ( m ) {
					   m = [ 0, m[2], m[3], m[1] ];

					} else {
						
						
						re2 = new RegExp("^([#.]?)(" + jQuery.chars + "*)");
						m = re2.exec(t);
					}

					m[2] = m[2].replace(/\\/g, "");

					var elem = ret[ret.length-1];

					
					if ( m[1] == "#" && elem && elem.getElementById ) {
						
						var oid = elem.getElementById(m[2]);
						
						
						
						
						if ( (jQuery.browser.msie||jQuery.browser.opera) && oid && typeof oid.id == "string" && oid.id != m[2] )
							oid = jQuery('[@id="'+m[2]+'"]', elem)[0];

						
						
						ret = r = oid && (!m[3] || jQuery.nodeName(oid, m[3])) ? [oid] : [];
					} else {
						
						for ( var i = 0; ret[i]; i++ ) {
							
							var tag = m[1] != "" || m[0] == "" ? "*" : m[2];

							
							if ( tag == "*" && ret[i].nodeName.toLowerCase() == "object" )
								tag = "param";

							r = jQuery.merge( r, ret[i].getElementsByTagName( tag ));
						}

						
						if ( m[1] == "." )
							r = jQuery.classFilter( r, m[2] );

						
						if ( m[1] == "#" ) {
							var tmp = [];

							
							for ( var i = 0; r[i]; i++ )
								if ( r[i].getAttribute("id") == m[2] ) {
									tmp = [ r[i] ];
									break;
								}

							r = tmp;
						}

						ret = r;
					}

					t = t.replace( re2, "" );
				}

			}

			
			if ( t ) {
				
				var val = jQuery.filter(t,r);
				ret = r = val.r;
				t = jQuery.trim(val.t);
			}
		}

		
		
		if ( t )
			ret = [];

		
		if ( ret && context == ret[0] )
			ret.shift();

		
		done = jQuery.merge( done, ret );

		return done;
	},

	classFilter: function(r,m,not){
		m = " " + m + " ";
		var tmp = [];
		for ( var i = 0; r[i]; i++ ) {
			var pass = (" " + r[i].className + " ").indexOf( m ) >= 0;
			if ( !not && pass || not && !pass )
				tmp.push( r[i] );
		}
		return tmp;
	},

	filter: function(t,r,not) {
		var last;

		
		while ( t  && t != last ) {
			last = t;

			var p = jQuery.parse, m;

			for ( var i = 0; p[i]; i++ ) {
				m = p[i].exec( t );

				if ( m ) {
					
					t = t.substring( m[0].length );

					
					if ( jQuery.expr[ m[1] ]._resort )
						m = jQuery.expr[ m[1] ]._resort( m );

					m[2] = m[2].replace(/\\/g, "");

					break;
				}
			}

			if ( !m )
				break;

			
			
			if ( m[1] == ":" && m[2] == "not" )
				r = jQuery.filter(m[3], r, true).r;

			
			else if ( m[1] == "." )
				r = jQuery.classFilter(r, m[2], not);

			
			else {
				var f = jQuery.expr[m[1]];
				if ( typeof f != "string" )
					f = jQuery.expr[m[1]][m[2]];

				
				eval("f = function(a,i){" +
					( jQuery.expr[ m[1] ]._prefix || "" ) +
					"return " + f + "}");

				
				r = jQuery.grep( r, f, not );
			}
		}

		
		
		return { r: r, t: t };
	},

	








	parents: function( elem ){
		var matched = [];
		var cur = elem.parentNode;
		while ( cur && cur != document ) {
			matched.push( cur );
			cur = cur.parentNode;
		}
		return matched;
	},
	
	











	nth: function(cur,result,dir,elem){
		result = result || 1;
		var num = 0;
		for ( ; cur; cur = cur[dir] ) {
			if ( cur.nodeType == 1 ) num++;
			if ( num == result || result == "even" && num % 2 == 0 && num > 1 && cur == elem ||
				result == "odd" && num % 2 == 1 && cur == elem ) break;
		}
		return cur;
	},
	
	








	sibling: function( n, elem ) {
		var r = [];

		for ( ; n; n = n.nextSibling ) {
			if ( n.nodeType == 1 && (!elem || n != elem) )
				r.push( n );
		}

		return r;
	}
});
