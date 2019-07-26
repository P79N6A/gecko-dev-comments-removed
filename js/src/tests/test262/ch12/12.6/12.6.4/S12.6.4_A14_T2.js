











for(x in function __func(){return {a:1};}()){
    var __reached = x;
};





if (__reached !== "a") {
	$ERROR('#2: function expession inside of for-in expression allowed');
}



