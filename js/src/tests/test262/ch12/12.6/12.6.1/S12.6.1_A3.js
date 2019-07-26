









__evaluated = eval("do __in__do=1; while (false)");



if (__in__do !== 1) {
	$ERROR('#1: __in__do === 1. Actual:  __in__do ==='+ __in__do  );
}





if (__evaluated !== 1) {
	$ERROR('#2: __evaluated === 1. Actual:  __evaluated ==='+ __evaluated  );
}



