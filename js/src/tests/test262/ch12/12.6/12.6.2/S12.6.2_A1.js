









var __in__do;

while ( false ) __in__do=1;



if (__in__do !== undefined) {
	$ERROR('#1: false evaluates to false');
}



while ( 0 ) __in__do=2;



if (__in__do !== undefined) {
	$ERROR('#2: 0 evaluates to false');
}



while ( "" ) __in__do=3;



if (__in__do !== undefined) {
	$ERROR('#3: empty string evaluates to false');
}



while ( null ) __in__do=4;



if (__in__do !== undefined) {
	$ERROR('#4: null evaluates to false');
}



while ( undefined ) __in__do=35;



if (__in__do !== undefined) {
	$ERROR('#5: undefined evaluates to false');
}



