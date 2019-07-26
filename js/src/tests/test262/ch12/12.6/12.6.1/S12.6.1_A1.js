









var __in__do;

do __in__do=1; while ( false );



if (__in__do!==1) {
	$ERROR('#1: false evaluates to false');
}



do __in__do=2; while ( 0 );



if (__in__do!==2) {
	$ERROR('#2: 0 evaluates to false');
}



do __in__do=3; while ( "" );



if (__in__do!==3) {
	$ERROR('#3: "" evaluates to false');
}



