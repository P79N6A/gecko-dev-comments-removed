













var __gunc = function(){};



if (typeof __gunc.prototype !== 'object') {
	$ERROR('#1: typeof __gunc.prototype === \'object\'. Actual: typeof __gunc.prototype ==='+typeof __gunc.prototype);
}





if (__gunc.prototype.constructor !== __gunc) {
	$ERROR('#2: __gunc.prototype.constructor === __gunc. Actual: __gunc.prototype.constructor ==='+__gunc.prototype.constructor);
}



var __constructor_was__enumed;

for (__prop in __gunc.prototype){
    if (__prop = 'constructor')
        __constructor_was__enumed = true;
}



if (__constructor_was__enumed) {
	$ERROR('#3: __constructor_was__enumed === false. Actual: __constructor_was__enumed ==='+__constructor_was__enumed);
}








