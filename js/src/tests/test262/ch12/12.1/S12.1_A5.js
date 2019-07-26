











try {
	throw 1;
    throw 2;
    throw 3;
    $ERROR('1.1: throw 1 lead to throwing exception');
} catch (e) {
	if (e!==1) {
		$ERROR('#1.2: Exception === 1. Actual:  Exception ==='+ e);
	}
}




try {
	{
	    throw 1;
        throw 2;
    }
    throw 3;
    $ERROR('#2.1: throw 1 lead to throwing exception');
} catch (e) {
	if (e!==1) {
		$ERROR('#2.2: Exception === 1. Actual:  Exception ==='+ e);
	}
}




try {
	throw 1;
    {
        throw 2;
        throw 3;
    }
    $ERROR('#3.1: throw 1 lead to throwing exception');
} catch (e) {
	if (e!==1) {
		$ERROR('#3.2: Exception === 1. Actual:  Exception ==='+ e);
	}
}



