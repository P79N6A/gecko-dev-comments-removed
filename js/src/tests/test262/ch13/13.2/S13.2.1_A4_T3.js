










function __func(){
    arguments[0]["PI"]=3.14;
}

var __obj={};

__func(__obj);



if (__obj.PI !== 3.14) {
	$ERROR('#1: __obj.PI === 3.14. Actual: __obj.PI ==='+__obj.PI);
}



