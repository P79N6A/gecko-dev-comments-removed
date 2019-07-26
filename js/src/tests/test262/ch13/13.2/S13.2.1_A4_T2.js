










function __func(__arg){
    __arg.foo="whiskey gogo";
}

var __obj={};

 __func(__obj);



if (__obj.foo !== "whiskey gogo") {
	$ERROR('#1: __obj.foo === "whiskey gogo". Actual: __obj.foo ==='+__obj.foo);
}



