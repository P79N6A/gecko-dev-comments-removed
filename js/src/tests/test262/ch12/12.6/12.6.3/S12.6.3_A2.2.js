











try {
	for((function(){throw "NoInExpression"})();;) {
		throw "Statement";
	}
	$ERROR('#1: (function(){throw "NoInExpression"})() lead to throwing exception');
} catch (e) {
	if (e !== "NoInExpression") {
		$ERROR('#1: When for (ExpressionNoIn ;  ; ) Statement is evaluated NoInExpression evaluates first');
	}
}



