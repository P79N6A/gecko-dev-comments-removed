









var x = (function __func(arg){return arg + arguments[1]})(1,"1");



if (x !== "11") {
	$ERROR('#1: x === "11". Actual: x ==='+x);
}






if (typeof __func !== 'undefined') {
	$ERROR('#2: typeof __func === \'undefined\'. Actual: typeof __func ==='+typeof __func);
}



