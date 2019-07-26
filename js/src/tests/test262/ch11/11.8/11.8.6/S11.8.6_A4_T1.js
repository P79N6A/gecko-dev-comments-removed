










if (false instanceof Boolean) {
	$ERROR('#1: false is not instanceof Boolean');
}


if (Boolean(false) instanceof Boolean) {
	$ERROR('#2: Boolean(false) is not instanceof Boolean');
}


if (new Boolean instanceof Boolean !== true) {
	$ERROR('#3: new Boolean instanceof Boolean');
}


