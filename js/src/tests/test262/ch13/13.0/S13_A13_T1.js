









function __func(__arg){
  delete arguments[0];
  if (arguments[0] !== undefined) {
    $ERROR('#1.1: arguments[0] === undefined');
  }
  return __arg;
}



if (__func(1) !== 1) {
	$ERROR('#1.2: __func(1) === 1. Actual: __func(1) ==='+__func(1));
}



