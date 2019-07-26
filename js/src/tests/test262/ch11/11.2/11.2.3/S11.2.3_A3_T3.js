










try {
  "1"();
    $ERROR('#1.1: "1"() throw TypeError. Actual: ' + ("1"()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: "1"() throw TypeError. Actual: ' + (e));	
  }
}


try {
  var x = "1";
  x();
    $ERROR('#2.1: var x = "1"; x() throw TypeError. Actual: ' + (x()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2.2: var x = "1"; x() throw TypeError. Actual: ' + (e));	
  }
}

