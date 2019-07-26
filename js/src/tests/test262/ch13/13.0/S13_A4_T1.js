









function __func(){return "zig-zig-sputnik";};



if (typeof __func !== "function") {
	$ERROR('#1: typeof __func === "function". Actual: typeof __func ==='+typeof __func);
}





if (__func() !== "zig-zig-sputnik") {
	$ERROR('#2: __func() === "zig-zig-sputnik". Actual: __func() ==='+__func());
}



