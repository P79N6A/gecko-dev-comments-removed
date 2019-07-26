













function __func(){};



if (typeof __func.prototype !== 'object') {
	$ERROR('#1: typeof __func.prototype === \'object\'. Actual: typeof __gunc.prototype ==='+typeof __gunc.prototype);
}





if (__func.prototype.constructor !== __func) {
	$ERROR('#2: __func.prototype.constructor === __func. Actual: __gunc.prototype.constructor ==='+__gunc.prototype.constructor);
}



var __constructor_was__enumed;

for (__prop in __func.prototype){
    if (__prop === 'constructor')
        __constructor_was__enumed = true;
}



if (__constructor_was__enumed) {
	$ERROR('#3: __constructor_was__enumed === false. Actual: __constructor_was__enumed ==='+__constructor_was__enumed);
}








