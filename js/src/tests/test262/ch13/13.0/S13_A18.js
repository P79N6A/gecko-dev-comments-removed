











if (typeof sinx !== 'undefined') {
	$ERROR('#1: typeof sinx === \'undefined\'. Actual: typeof sinx ==='+typeof sinx);
}



var __val = function derivative(f, dx) {
    return function(x) {
      return (f(x + dx) - f(x)) / dx;
    };
}(function sinx(x){return Math.sin(x);},.0001)(0.5);



if (typeof sinx !== 'undefined') {
	$ERROR('#2: typeof sinx === \'undefined\'. Actual: typeof sinx ==='+typeof sinx);
}



