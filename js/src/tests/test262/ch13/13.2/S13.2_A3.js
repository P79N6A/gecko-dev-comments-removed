









function __func(){};



if (__func.length !== 0) {
	$ERROR('#1: __func.length === 0. Actual: __func.length ==='+__func.length);
}



function __gunc(a,b,c){};



if (__gunc.length !== 3) {
	$ERROR('#2: __gunc.length === 3. Actual: __gunc.length ==='+__gunc.length);
}




