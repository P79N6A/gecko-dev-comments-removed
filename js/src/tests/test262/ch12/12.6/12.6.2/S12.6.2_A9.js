











try {
	while(x!=1) {
	    var x = 1; 
	    abaracadabara;
	};
	$ERROR('#1: "abbracadabra" lead to throwing exception');

} catch (e) {
    if (e instanceof Test262Error) throw e;
}

if (x !== 1) {
	$ERROR('#1.1: while statement evaluates as is, without syntax checks');
}



