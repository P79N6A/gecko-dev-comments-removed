










try {
  true();
    $ERROR('#1.1: true() throw TypeError. Actual: ' + (true()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: true() throw TypeError. Actual: ' + (e));	
  }
}


try {
  var x = true;
  x();
    $ERROR('#2.1: var x = true; x() throw TypeError. Actual: ' + (x()))
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2.2: var x = true; x() throw TypeError. Actual: ' + (e))  
  }
}

