










if (0 instanceof Number) {
	$ERROR('#1: 0 is not instanceof Number');
}


if (Number(0) instanceof Number) {
	$ERROR('#2: Number(0) is not instanceof Number');
}


if (new Number instanceof Number !== true) {
	$ERROR('#3: new Number instanceof Number');
}


