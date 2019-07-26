










try {
  new Boolean(true)();
  $ERROR('#1.1: new Boolean(true)() throw TypeError. Actual: ' + (new Boolean(true)()));  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: new Boolean(true)() throw TypeError. Actual: ' + (e));  
  }
}


try {
  var x = new Boolean(true);
  x();
  $ERROR('#2.1: var x = new Boolean(true); x() throw TypeError. Actual: ' + (x()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2.2: var x = new Boolean(true); x() throw TypeError. Actual: ' + (e));	
  }
}


