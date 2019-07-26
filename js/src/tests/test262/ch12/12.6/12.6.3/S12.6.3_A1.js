









var __in__for = 0;



try {
	for (;;){
    
    if(++__in__for>100)throw 1;
}
} catch (e) {
	if (e !== 1) {
		$ERROR('#1: for {;;} is admitted and leads to infinite loop');
	}
}





if (__in__for !== 101) {
	$ERROR('#2: __in__for === 101. Actual:  __in__for ==='+ __in__for  );
}




