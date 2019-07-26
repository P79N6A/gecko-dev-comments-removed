











if (typeof __func !== "undefined") {
	$ERROR('#1: typeof __func === "undefined"');
}



var __obj = new function __func(arg){this.prop=arg; return {feat: ++arg}}(5);



if (__obj.prop !== undefined) {
	$ERROR('#2: __obj.prop === undefined. Actual: __obj.prop ==='+__obj.prop);
}





if (__obj.feat !== 6) {
	$ERROR('#3: __obj.feat === 6. Actual: __obj.feat ==='+__obj.feat);
}





if (typeof __func !== "undefined") {
	$ERROR('#4: typeof __func === "undefined". Actual: typeof __func ==='+typeof __func);
}



