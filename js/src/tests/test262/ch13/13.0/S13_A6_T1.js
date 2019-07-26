









function __func(){return 1};
 
var __store__func = __func;
 
var __1 = __func();
 
 function __func(){return 'A'};
 
var __A = __func();
 


if (__store__func !== __func) {
	$ERROR('#1: __store__func === __func. Actual: __store__func ==='+__store__func);
}


 


if (__1 !== __A) {
	$ERROR('#2: __1 === __A. Actual: __1 ==='+__1);
}



