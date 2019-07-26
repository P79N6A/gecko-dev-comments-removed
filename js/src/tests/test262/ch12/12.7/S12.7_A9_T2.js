









var x=0,y=0;

(function(){
FOR : for(;;){
	try{
		x++;
		if(x===10)return;
		throw 1;
	} catch(e){
		continue;
	}	
}
})();



if (x!==10) {
	$ERROR('#1: Continue inside of try-catch nested in loop is allowed');
}



