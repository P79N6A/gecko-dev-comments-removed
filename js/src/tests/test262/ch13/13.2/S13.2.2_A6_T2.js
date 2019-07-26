













var __FOO="fooValue";
var __BAR="barValue";

function __func (arg){
	this.foo=arg;
    return true;
	this.bar=arguments[1];
};

var __obj = new __func(__FOO, __BAR);



if (__obj.foo!==__FOO) {
	$ERROR('#1: __obj.foo === __FOO. Actual: __obj.foo==='+__obj.foo);
}





if (__obj.bar!==undefined) {
	$ERROR('#2: __obj.bar === undefined. Actual: __obj.bar==='+__obj.bar);
}



