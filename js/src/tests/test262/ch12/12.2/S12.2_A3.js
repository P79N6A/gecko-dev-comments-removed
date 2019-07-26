









var __var = "OUT";

(function(){
    var __var ="IN";
	(function(){__var = "INNER_SPACE";})();
	(function(){var __var = "INNER_SUN";})();
	
	
    if (__var !== "INNER_SPACE") {
    	$ERROR('#1: __var === "INNER_SPACE". Actual:  __var ==='+ __var  );
    }
	
	
})();



if (__var !== "OUT") {
	$ERROR('#2: __var === "OUT". Actual:  __var ==='+ __var  );
}




(function(){
    __var ="IN";
	(function(){__var = "INNERED"})();
	(function(){var __var = "INNAGER"})();
	
	
    if (__var!=="INNERED") {
    	$ERROR('#3: __var==="INNERED". Actual:  __var==='+ __var );
    }
	
	
})();



if (__var!=="INNERED") {
	$ERROR('#4: __var==="INNERED". Actual:  __var==='+ __var );
}



