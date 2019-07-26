











if (delete(__variable)) {
	$ERROR('#1: delete(__variable)===false');
}





if (delete(this["__variable"])) {
	$ERROR('#2: delete(this["__variable"])===false');
}




var __variable;
var __variable = "defined";



if (delete(__variable) | delete(this["__variable"])) {
	$ERROR('#3: (delete(__variable) | delete(this["__variable"]))===false' );
}





if ((__variable !== "defined")|(this["__variable"] !=="defined")) {
	$ERROR('#4: __variable === "defined" and this["__variable"] ==="defined"');
}




