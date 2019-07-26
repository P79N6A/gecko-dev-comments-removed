









var __t__err = new TypeError;


if (!(__t__err instanceof Error)) {
	$ERROR('#1: TypeError is subclass of Error from instanceof operator poit of view');
}


if (!(__t__err instanceof TypeError)) {
	$ERROR('#2: TypeError is subclass of Error from instanceof operator poit of view');
}


var err__t__ = TypeError('failed');


if (!(err__t__ instanceof Error)) {
	$ERROR('#3: TypeError is subclass of Error from instanceof operator poit of view');
}


if (!(err__t__ instanceof TypeError)) {
	$ERROR('#4: TypeError is subclass of Error from instanceof operator poit of view');
}


