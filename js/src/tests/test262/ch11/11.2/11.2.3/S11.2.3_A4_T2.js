










try {
  new Number(1)();
  $ERROR('#1.1: new Number(1)() throw TypeError. Actual: ' + (new Number(1)()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: new Number(1)() throw TypeError. Actual: ' + (e));	
  }
}


try {
  var x = new Number(1);
  x();
  $ERROR('#2.1: var x = new Number(1); x() throw TypeError. Actual: ' + (x()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2.2: var x = new Number(1); x() throw TypeError. Actual: ' + (e));	
  }
}


