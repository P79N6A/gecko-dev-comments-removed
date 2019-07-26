









function __func__INC(arg){return arg + 1;};
function __func__MULT(incrementator, arg, mult){ return incrementator(arg)*mult; };



if (__func__MULT(__func__INC, 2, 2) !== 6) {
	$ERROR('#1: function  can be passed as argument');
}




