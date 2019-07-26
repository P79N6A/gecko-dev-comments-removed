











if (typeof __func !== "function") {
	$ERROR('#1: typeof __func === "function". Actual:  typeof __func ==='+ typeof __func  );
}





if (typeof __gunc !== "undefined") {
	$ERROR('#2: typeof __gunc === "undefined". Actual:  typeof __gunc ==='+ typeof __gunc  );
}



function __func(){
    function __gunc(){return true};
}

