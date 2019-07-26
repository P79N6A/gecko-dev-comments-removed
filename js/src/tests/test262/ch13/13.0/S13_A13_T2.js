









function __func(__arg){
  __arg = 2;
  delete arguments[0];
  if (arguments[0] !== undefined) {
    $ERROR('#1.1: arguments[0] === undefined');
  }
  return __arg;
}



if (__func(1) !== 2) {
	$ERROR('#1.2: __func(1) === 2. Actual: __func(1) ==='+__func(1));
}



