











try{
	eval("function __func(){/ ABC}");
	$ERROR('#1: eval("function __func(){/ ABC}") lead to throwing exception');
} catch(e){
	if(!(e instanceof SyntaxError)){
		$ERROR('#1.1: eval("function __func(){/ ABC}") lead to throwing exception of SyntaxError. Actual: exception is '+e);
	}
}





try{
	eval("function __func(){&1}");
	$ERROR('#3: eval("function __func(){&1}") lead to throwing exception');
} catch(e){
	if(!(e instanceof SyntaxError)){
		$ERROR('#3.1: eval("function __func(){&1}") lead to throwing exception of SyntaxError. Actual: exception is '+e);
	}
}





try{
	eval("function __func(){# ABC}");
	$ERROR('#4: eval("function __func(){# ABC}") lead to throwing exception');
} catch(e){
	if(!(e instanceof SyntaxError)){
		$ERROR('#4.1: eval("function __func(){# ABC}") lead to throwing exception of SyntaxError. Actual: exception is '+e);
	}
}



