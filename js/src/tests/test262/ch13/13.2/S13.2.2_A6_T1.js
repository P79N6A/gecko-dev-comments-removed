













__FOO="fooValue";
__BAR="barValue";

__func = function(arg){
	this.foo=arg;
    return 0;
	this.bar=arguments[1];
};

__obj = new __func(__FOO, __BAR);



if (__obj.foo!==__FOO) {
	$ERROR('#1: __obj.foo === __FOO. Actual: __obj.foo==='+__obj.foo);
}





if (__obj.bar!==undefined) {
	$ERROR('#2: __obj.bar === undefined. Actual: __obj.bar==='+__obj.bar);
}



