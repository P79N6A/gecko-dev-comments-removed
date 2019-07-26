













var __FRST="one";
var __SCND="two";

var __func = function(arg1, arg2){
	this.first=arg1;
	
	__gunc.prop=arg2;
    return __gunc;
    
    function __gunc(arg){return ++arg};
	
};

var __instance = new __func(__FRST, __SCND);



if (__instance.first !== undefined) {
	$ERROR('#1: __instance.first === undefined. Actual: __instance.first ==='+__instance.first);
}





if (__instance.prop!==__SCND) {
	$ERROR('#2: __instance.prop === __SCND. Actual: __instance.prop ==='+__instance.prop);
}





if (__instance(1)!== 2) {
	$ERROR('#2: __instance(1)=== 2. Actual: __instance(1) ==='+__instance(1));
}




