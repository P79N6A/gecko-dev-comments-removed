











try {
	var __evaluated = eval("if(1);");
	if (__evaluated !== undefined) {
		$ERROR('#1: __evaluated === undefined. Actual:  __evaluated ==='+ __evaluated  );
	}

} catch (e) {
	$ERROR('#1.1: "__evaluated = eval("if(1);")" does not lead to throwing exception');

}



