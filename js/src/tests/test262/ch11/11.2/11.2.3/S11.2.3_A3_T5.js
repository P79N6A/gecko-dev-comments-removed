










try {
  null();
    $ERROR('#1.1: null() throw TypeError. Actual: ' + (null()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: null() throw TypeError. Actual: ' + (e));	
  }
}


try {
  var x = null;
  x();
    $ERROR('#2.1: var x = null; x() throw TypeError. Actual: ' + (x()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2.2: var x = null; x() throw TypeError. Actual: ' + (e));	
  }
}

