









 function __func() {
 	return arguments.length;
 }
 


if (__func('A') !== 1) {
 	$ERROR('#1: __func(\'A\') === 1. Actual: __func(\'A\') ==='+__func('A'));
}





if (__func('A', 'B', 1, 2,__func) !== 5) {
	$ERROR('#2: __func(\'A\', \'B\', 1, 2,__func) === 5. Actual: __func(\'A\', \'B\', 1, 2,__func) ==='+__func('A', 'B', 1, 2,__func));
}





if (__func() !== 0) {
	$ERROR('#3: __func() === 0. Actual: __func() ==='+__func());
}



