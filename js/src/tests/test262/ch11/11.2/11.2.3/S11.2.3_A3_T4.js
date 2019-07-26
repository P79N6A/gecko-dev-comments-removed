










try {
  undefined();
    $ERROR('#1.1: undefined() throw TypeError. Actual: ' + (e));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: undefined() throw TypeError. Actual: ' + (e));	
  }
}


try {
  var x = undefined;
  x();
    $ERROR('#2.1: var x = undefined; x() throw TypeError. Actual: ' + (e));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2.2: var x = undefined; x() throw TypeError. Actual: ' + (e));	
  }
}

