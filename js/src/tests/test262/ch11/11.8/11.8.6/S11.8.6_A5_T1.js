









var __err = new Error;



if (!(__err instanceof Error)) {
	$ERROR('#1: TypeError is subclass of Error from instanceof operator poit of view');
}





if (__err instanceof TypeError) {
	$ERROR('#2: TypeError is subclass of Error from instanceof operator poit of view');
}



var err__ = Error('failed');



if (!(err__ instanceof Error)) {
	$ERROR('#3: TypeError is subclass of Error from instanceof operator poit of view');
}





if (err__ instanceof TypeError) {
	$ERROR('#4: TypeError is subclass of Error from instanceof operator poit of view');
}




