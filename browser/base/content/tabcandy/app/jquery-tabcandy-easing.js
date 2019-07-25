







































jQuery.easing['jswing'] = jQuery.easing['swing'];

jQuery.extend( jQuery.easing,
{
	def: 'easeOutQuad',
	swing: function (x, t, b, c, d) {
		
		return jQuery.easing[jQuery.easing.def](x, t, b, c, d);
	},
	tabcandyBounce: function (x, t, b, c, d, s) {
		if (s == undefined) s = 1.70158;
		return c*(Math.pow((t=t/d-1),3)*t*((s+1)*t+.3 + s) + 1) + b;
	}
});

