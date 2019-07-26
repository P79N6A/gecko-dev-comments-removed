











try{
    var __result = __func();
	$ERROR("#1: var __result = __func() lead to throwing exception");
} catch(e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: func should throw a TypeError  Actual: ' + (e));  
  }
}




var __func = function __func(){return "ONE";};



var __result = __func();
if (__result !== "ONE") {
	$ERROR('#2: __result === "ONE". Actual: __result ==='+__result);
}



__func = function __func(){return "TWO";};



var __result = __func();
if (__result !== "TWO") {
	$ERROR('#3: __result === "TWO". Actual: __result ==='+__result);
}



