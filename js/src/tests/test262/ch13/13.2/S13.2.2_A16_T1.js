











if (typeof __func !== "undefined") {
	$ERROR('#1: typeof __func === "undefined". Actual: typeof __func ==='+typeof __func);
}



var __obj = new function __func(){this.prop=1;};



if (__obj.prop !== 1) {
	$ERROR('#2: __obj.prop === 1. Actual: __obj.prop ==='+__obj.prop);
}





if (typeof __func !== "undefined") {
	$ERROR('#5: typeof __func === "undefined". Actual: typeof __func ==='+typeof __func);
}



