









__obj={aa:1,ba:2,ca:3};

__accum="";

for (__key in __obj){
	
    erasator_T_1000(__obj,"b");
  
	__accum+=(__key+__obj[__key]);
	
}




if (!((__accum.indexOf("aa1")!==-1)&&(__accum.indexOf("ca3")!==-1))) {
	$ERROR('#1: (__accum.indexOf("aa1")!==-1)&&(__accum.indexOf("ca3")!==-1)');
}





if (__accum.indexOf("ba2")!==-1) {
	$ERROR('#2: __accum.indexOf("ba2") === -1. Actual:  __accum.indexOf("ba2") ==='+ __accum.indexOf("ba2")  );
}





function erasator_T_1000(hash_map, charactr){
	for (key in hash_map){
		if (key.indexOf(charactr)===0) {
			delete hash_map[key];
		};
	}
}

