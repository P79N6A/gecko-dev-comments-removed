













var __FRST="one";
var __SCND="two";

function __func (arg1, arg2){
	this.first=arg1;
	var __obj={second:arg2};
    return __obj;
	
};

var __obj__ = new __func(__FRST, __SCND);



if (__obj__.first !== undefined) {
	$ERROR('#1: __obj__.first === undefined. Actual: __obj__.first==='+__obj__.first);
}





if (__obj__.second !== __SCND) {
	$ERROR('#2: __obj__.second === __SCND. Actual: __obj__.second ==='+__obj__.second);
}



