











if (typeof __func !== "undefined") {
	$ERROR('#1: typeof __func === "undefined". Actual: typeof __func ==='+typeof __func);
}



var __obj = new function __func(arg){this.prop=arg;}(5);



if (__obj.prop !== 5) {
	$ERROR('#2: __obj.prop === 5. Actual: __obj.prop ==='+__obj.prop);
}





if (typeof __func !== "undefined") {
	$ERROR('#3: typeof __func === "undefined". Actual: typeof __func ==='+typeof __func);
}



