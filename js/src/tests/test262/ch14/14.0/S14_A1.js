









var THERE = "I'm there";
var HERE = "I'm here";



if ( __func !== undefined) {
	$ERROR('#1: __func === undefined. Actual:  __func ==='+ __func  );
}



if (true){
    var __func = function(){return HERE;};
} else {
    var __func = function (){return THERE;};
};



if (__func() !== HERE) {
	$ERROR('#2: __func() === HERE. Actual:  __func() ==='+ __func()  );
}



