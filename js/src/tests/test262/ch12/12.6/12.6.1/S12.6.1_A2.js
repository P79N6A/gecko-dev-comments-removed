









try {
	do __in__do = "reached"; while (abbracadabra);
	$ERROR('#1: \'do __in__do = "reached"; while (abbracadabra)\' lead to throwing exception');
} catch (e) {
    if (e instanceof Test262Error) throw e;
}



if (__in__do !== "reached") {
	$ERROR('#1.1: __in__do === "reached". Actual:  __in__do ==='+ __in__do  );
}





