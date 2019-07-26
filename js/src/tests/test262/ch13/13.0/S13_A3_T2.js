










var __func = function (arg){
    if (arg === 1) {
    	return arg;
    } else {
    	return __func(arg-1)*arg;
    }
};

var fact_of_3 =  __func(3);



if (fact_of_3 !== 6) {
	$ERROR("#1: fact_of_3 === 6. Actual: fact_of_3 ==="+fact_of_3);
}



