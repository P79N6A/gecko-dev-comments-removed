











try {
	__func=__func;
	$ERROR('#1: "__func=__func" lead to throwing exception');
} catch (e) {
	;
}






try {
	if(function __func(){throw "FunctionExpression";}) (function(){throw "TrueBranch"})(); else (function(){"MissBranch"})();
} catch (e) {
	if (e !== "TrueBranch") {
		$ERROR('#2: Exception ==="TrueBranch". Actual:  Exception ==='+ e);
	}
}





try {
	__func=__func;
	$ERROR('#3: "__func=__func" lead to throwing exception');
} catch (e) {
	;
}






