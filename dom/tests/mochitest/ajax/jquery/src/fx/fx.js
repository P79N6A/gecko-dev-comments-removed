jQuery.fn.extend({

	










	
	



















	show: function(speed,callback){
		return speed ?
			this.animate({
				height: "show", width: "show", opacity: "show"
			}, speed, callback) :
			
			this.filter(":hidden").each(function(){
				this.style.display = this.oldblock ? this.oldblock : "";
				if ( jQuery.css(this,"display") == "none" )
					this.style.display = "block";
			}).end();
	},
	
	










	
	



















	hide: function(speed,callback){
		return speed ?
			this.animate({
				height: "hide", width: "hide", opacity: "hide"
			}, speed, callback) :
			
			this.filter(":visible").each(function(){
				this.oldblock = this.oldblock || jQuery.css(this,"display");
				if ( this.oldblock == "none" )
					this.oldblock = "block";
				this.style.display = "none";
			}).end();
	},

	
	_toggle: jQuery.fn.toggle,
	
	












	toggle: function( fn, fn2 ){
		return jQuery.isFunction(fn) && jQuery.isFunction(fn2) ?
			this._toggle( fn, fn2 ) :
			fn ?
				this.animate({
					height: "toggle", width: "toggle", opacity: "toggle"
				}, fn, fn2) :
				this.each(function(){
					jQuery(this)[ jQuery(this).is(":hidden") ? "show" : "hide" ]();
				});
	},
	
	




















	slideDown: function(speed,callback){
		return this.animate({height: "show"}, speed, callback);
	},
	
	




















	slideUp: function(speed,callback){
		return this.animate({height: "hide"}, speed, callback);
	},

	




















	slideToggle: function(speed, callback){
		return this.animate({height: "toggle"}, speed, callback);
	},
	
	





















	fadeIn: function(speed, callback){
		return this.animate({opacity: "show"}, speed, callback);
	},
	
	





















	fadeOut: function(speed, callback){
		return this.animate({opacity: "hide"}, speed, callback);
	},
	
	






















	fadeTo: function(speed,to,callback){
		return this.animate({opacity: to}, speed, callback);
	},
	
	



































	animate: function( prop, speed, easing, callback ) {
		return this.queue(function(){
			var hidden = jQuery(this).is(":hidden"),
				opt = jQuery.speed(speed, easing, callback),
				self = this;
			
			for ( var p in prop )
				if ( prop[p] == "hide" && hidden || prop[p] == "show" && !hidden )
					return jQuery.isFunction(opt.complete) && opt.complete.apply(this);

			this.curAnim = jQuery.extend({}, prop);
			
			jQuery.each( prop, function(name, val){
				var e = new jQuery.fx( self, opt, name );
				if ( val.constructor == Number )
					e.custom( e.cur(), val );
				else
					e[ val == "toggle" ? hidden ? "show" : "hide" : val ]( prop );
			});
		});
	},
	
	



	queue: function(type,fn){
		if ( !fn ) {
			fn = type;
			type = "fx";
		}
	
		return this.each(function(){
			if ( !this.queue )
				this.queue = {};
	
			if ( !this.queue[type] )
				this.queue[type] = [];
	
			this.queue[type].push( fn );
		
			if ( this.queue[type].length == 1 )
				fn.apply(this);
		});
	}

});

jQuery.extend({
	
	speed: function(speed, easing, fn) {
		var opt = speed && speed.constructor == Object ? speed : {
			complete: fn || !fn && easing || 
				jQuery.isFunction( speed ) && speed,
			duration: speed,
			easing: fn && easing || easing && easing.constructor != Function && easing || (jQuery.easing.swing ? "swing" : "linear")
		};

		opt.duration = (opt.duration && opt.duration.constructor == Number ? 
			opt.duration : 
			{ slow: 600, fast: 200 }[opt.duration]) || 400;
	
		
		opt.old = opt.complete;
		opt.complete = function(){
			jQuery.dequeue(this, "fx");
			if ( jQuery.isFunction( opt.old ) )
				opt.old.apply( this );
		};
	
		return opt;
	},
	
	easing: {
		linear: function( p, n, firstNum, diff ) {
			return firstNum + diff * p;
		},
		swing: function( p, n, firstNum, diff ) {
			return ((-Math.cos(p*Math.PI)/2) + 0.5) * diff + firstNum;
		}
	},
	
	queue: {},
	
	dequeue: function(elem,type){
		type = type || "fx";
	
		if ( elem.queue && elem.queue[type] ) {
			
			elem.queue[type].shift();
	
			
			var f = elem.queue[type][0];
		
			if ( f ) f.apply( elem );
		}
	},

	timers: [],

	




	
	fx: function( elem, options, prop ){

		var z = this;

		
		var y = elem.style;
		
		if ( prop == "height" || prop == "width" ) {
			
			var oldDisplay = jQuery.css(elem, "display");

			
			var oldOverflow = y.overflow;
			y.overflow = "hidden";
		}

		
		z.a = function(){
			if ( options.step )
				options.step.apply( elem, [ z.now ] );

			if ( prop == "opacity" )
				jQuery.attr(y, "opacity", z.now); 
			else {
				y[prop] = parseInt(z.now) + "px";
				y.display = "block"; 
			}
		};

		
		z.max = function(){
			return parseFloat( jQuery.css(elem,prop) );
		};

		
		z.cur = function(){
			var r = parseFloat( jQuery.curCSS(elem, prop) );
			return r && r > -10000 ? r : z.max();
		};

		
		z.custom = function(from,to){
			z.startTime = (new Date()).getTime();
			z.now = from;
			z.a();

			jQuery.timers.push(function(){
				return z.step(from, to);
			});

			if ( jQuery.timers.length == 1 ) {
				var timer = setInterval(function(){
					var timers = jQuery.timers;
					
					for ( var i = 0; i < timers.length; i++ )
						if ( !timers[i]() )
							timers.splice(i--, 1);

					if ( !timers.length )
						clearInterval( timer );
				}, 13);
			}
		};

		
		z.show = function(){
			if ( !elem.orig ) elem.orig = {};

			
			elem.orig[prop] = jQuery.attr( elem.style, prop );

			options.show = true;

			
			z.custom(0, this.cur());

			
			
			if ( prop != "opacity" )
				y[prop] = "1px";
			
			
			jQuery(elem).show();
		};

		
		z.hide = function(){
			if ( !elem.orig ) elem.orig = {};

			
			elem.orig[prop] = jQuery.attr( elem.style, prop );

			options.hide = true;

			
			z.custom(this.cur(), 0);
		};

		
		z.step = function(firstNum, lastNum){
			var t = (new Date()).getTime();

			if (t > options.duration + z.startTime) {
				z.now = lastNum;
				z.a();

				if (elem.curAnim) elem.curAnim[ prop ] = true;

				var done = true;
				for ( var i in elem.curAnim )
					if ( elem.curAnim[i] !== true )
						done = false;

				if ( done ) {
					if ( oldDisplay != null ) {
						
						y.overflow = oldOverflow;
					
						
						y.display = oldDisplay;
						if ( jQuery.css(elem, "display") == "none" )
							y.display = "block";
					}

					
					if ( options.hide )
						y.display = "none";

					
					if ( options.hide || options.show )
						for ( var p in elem.curAnim )
							jQuery.attr(y, p, elem.orig[p]);
				}

				
				if ( done && jQuery.isFunction( options.complete ) )
					
					options.complete.apply( elem );

				return false;
			} else {
				var n = t - this.startTime;
				
				var p = n / options.duration;
				
				
				z.now = jQuery.easing[options.easing](p, n, firstNum, (lastNum-firstNum), options.duration);

				
				z.a();
			}

			return true;
		};
	
	}
});
