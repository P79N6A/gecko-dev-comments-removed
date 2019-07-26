











try {
	if (true) (function(){throw "instatement"})();
	$FAIL("#1 failed")
} catch (e) {
	if (e !== "instatement") {
		$ERROR('#1: Exception === "instatement". Actual:  Exception ==='+ e);
	}
}





try {
	if (false) (function(){throw "truebranch"})(); (function(){throw "missbranch"})();
	$FAIL("#2 failed")
} catch (e) {
	if (e !== "missbranch") {
		$ERROR('#2: Exception === "missbranch". Actual:  Exception ==='+ e);
	}
}




