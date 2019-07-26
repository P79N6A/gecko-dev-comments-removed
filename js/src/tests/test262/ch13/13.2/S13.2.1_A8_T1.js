











function __func(){
    var x = 1;
    throw ("Catch Me If You Can")
    return x;
}

try{
    var x=__func()
    $ERROR('#0: var x=__func() lead to throwing exception');
} catch(e){
    if (e !== "Catch Me If You Can") {
    	$ERROR('#1: Exception === "Catch Me If You Can". Actual: exception ==='+e);
    }
}

