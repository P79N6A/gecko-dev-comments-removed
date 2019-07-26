










try {
  var x = 1;
  var z = (x <<= y);
  $ERROR('#1.1: var x = 1; x <<= y throw ReferenceError. Actual: ' + (z));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: var x = 1; x <<= y throw ReferenceError. Actual: ' + (e));  
  }
}

