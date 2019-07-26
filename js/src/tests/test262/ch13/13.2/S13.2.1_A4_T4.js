










function __func(){
    arguments[0]["E"]=2.74;
}

var __obj={};

__func(__obj);



if (__obj.E !== 2.74) {
	$ERROR('#1: __obj.E === 2.74. Actual: __obj.E ==='+__obj.E);
}



