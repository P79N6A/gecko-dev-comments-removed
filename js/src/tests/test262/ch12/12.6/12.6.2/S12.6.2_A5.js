









__evaluated = eval("while(1) {__in__do__before__break=1; break; __in__do__after__break=2;}");



if (__in__do__before__break !== 1) {
	$ERROR('#1: __in__do__before__break === 1. Actual:  __in__do__before__break ==='+ __in__do__before__break  );
}





if (typeof __in__do__after__break !== "undefined") {
	$ERROR('#2: typeof __in__do__after__break === "undefined". Actual:  typeof __in__do__after__break ==='+ typeof __in__do__after__break  );
}





if (__evaluated !== 1) {
	$ERROR('#3: __evaluated === 1. Actual:  __evaluated ==='+ __evaluated  );
}



