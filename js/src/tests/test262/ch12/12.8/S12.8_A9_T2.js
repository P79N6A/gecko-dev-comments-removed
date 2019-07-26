









var x=0,y=0;

(function(){
FOR : for(;;){
	try{
		x++;
		if(x===10)return;
		throw 1;
	} catch(e){
		break ;
	}	
}
})();



if (x!==1) {
	$ERROR('#1: break inside of try-catch nested in loop is allowed');
}



