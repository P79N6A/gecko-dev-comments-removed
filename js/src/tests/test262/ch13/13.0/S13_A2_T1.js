









var x = (function __func(arg){return arg})(1);



if (x !== 1) {
	$ERROR('#1: x === 1. Actual: x ==='+x);
}






if (typeof __func !== 'undefined') {
	$ERROR('#2: typeof __func === \'undefined\'. Actual: typeof __func ==='+typeof __func);
}



