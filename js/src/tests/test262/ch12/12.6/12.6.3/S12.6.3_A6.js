











try {
	for(;;(function(){throw "SecondExpression";})()){
        var __in__for = "reached";
    }
    $ERROR('#1: (function(){throw "SecondExpression"}() lead to throwing exception');
} catch (e) {
	if (e !== "SecondExpression") {
		$ERROR('#1: When for ( ;  ; Expression) Statement is evaluated Statement evaluates first then Expression evaluates');
	}
}





if (__in__for !== "reached") {
	$ERROR('#2: __in__for === "reached". Actual:  __in__for ==='+ __in__for  );
}



