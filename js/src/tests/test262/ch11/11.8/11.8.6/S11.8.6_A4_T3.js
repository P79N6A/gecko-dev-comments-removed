










if ("" instanceof String) {
	$ERROR('#1: "" is not instanceof String');
}


if (String("") instanceof String) {
	$ERROR('#2: String("") is not instanceof String');
}


if (new String instanceof String !== true) {
	$ERROR('#3: new String instanceof String');
}

