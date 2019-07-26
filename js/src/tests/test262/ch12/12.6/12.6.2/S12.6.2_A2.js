









try {
	while ((function(){throw 1})()) __in__while = "reached"; 
	$ERROR('#1: \'while ((function(){throw 1})()) __in__while = "reached"\' lead to throwing exception');
} catch (e) {
	if (e !== 1) {
		$ERROR('#1: Exception === 1. Actual:  Exception ==='+e);
	}
}



if (typeof __in__while !== "undefined") {
	$ERROR('#1.1: typeof __in__while === "undefined". Actual: typeof __in__while ==='+typeof __in__while);
}





