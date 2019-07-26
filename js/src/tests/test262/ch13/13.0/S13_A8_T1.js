









 function __func(param1, param2, param3) {
 	return arguments.length;
 }
 


if (__func('A') !== 1) {
 	$ERROR('#1: __func(\'A\') === 1. Actual: __func(\'A\') ==='+__func('A'));
}





if (__func('A', 'B', 1, 2,__func) !== 5) {
	$ERROR('#2: __func(\'A\', \'B\', 1, 2,__func) === 5. Actual: __func(\'A\', \'B\', 1, 2,__func) ==='+__func('A', 'B', 1, 2,__func));
}


 
 
 

