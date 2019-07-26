









var __condition=0

__evaluated = eval("do eval(\"__condition++\"); while (__condition<5)");



if (__condition !== 5) {
	$ERROR('#1: The "do-while" statement is evaluted according to the Standard ');
}





if (__evaluated !== 4) {
	$ERROR('#2: The "do-while" statement returns (normal, V, empty)');
}




