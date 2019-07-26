









var __condition=0

__evaluated = eval("while (__condition<5) eval(\"__condition++\");");



if (__condition !== 5) {
	$ERROR('#1: The "while" statement is evaluated as described in the Standard');
}





if (__evaluated !== 4) {
	$ERROR('#2: The "while" statement returns (normal, V, empty)');
}




