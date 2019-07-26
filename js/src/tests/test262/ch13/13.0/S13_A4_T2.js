









function __func(arg){return ++arg;};



if (typeof __func !== "function") {
	$ERROR('#1: typeof __func === "function". Actual: typeof __func ==='+typeof __func);
}





if (__func(1) !== 2) {
	$ERROR('#2: __func(1) === 2. Actual: __func(1) ==='+__func(1));
}



