











try {
	if ((function(){throw 1})()) abracadabra
} catch (e) {
	if (e !== 1) {
		$ERROR('#1: Exception === 1. Actual:  Exception ==='+ e);
	}
}





try {
	if ((function(){throw 1})()) abracadabra; else blablachat;
} catch (e) {
	if (e !== 1) {
		$ERROR('#2: Exception === 1. Actual:  Exception ==='+ e);
	}
}




