









function __func(__arg){
  __arg = 2;
  delete arguments[0];
  if (arguments[0] !== undefined) {
    $ERROR('#1.1: arguments[0] === undefined');
  }
  arguments[0] = "A";
  if (arguments[0] !== "A") {
    $ERROR('#1.2: arguments[0] === "A"');
  }
  return __arg;
}



if (__func(1) !== 2) {
	$ERROR('#1.3: __func(1) === 2. Actual: __func(1) ==='+__func(1));
}



