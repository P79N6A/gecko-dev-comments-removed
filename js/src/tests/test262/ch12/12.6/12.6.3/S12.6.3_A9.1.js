









supreme=5;



try {
	var __evaluated =  eval("for(count=0;;) {if (count===supreme)break;else count++; }");
	if (__evaluated !== 4) {
		$ERROR('#1: __evaluated === 4. Actual:  __evaluated ==='+ __evaluated  );
	}
} catch (e) {
	$ERROR('#1: var __evaluated =  eval("for(count=0;;) {if (count===supreme)break;else count++; }"); does not lead to throwing exception');
}



