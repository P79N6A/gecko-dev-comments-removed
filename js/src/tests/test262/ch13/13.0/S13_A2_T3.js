









var x = (function __func(){return arguments[0] +"-"+ arguments[1]})("Obi","Wan");



if (x !== "Obi-Wan") {
	$ERROR('#1: x === "Obi-Wan". Actual: x ==='+x);
}






if (typeof __func !== 'undefined') {
	$ERROR('#2: typeof __func === \'undefined\'. Actual: typeof __func ==='+typeof __func);
}



