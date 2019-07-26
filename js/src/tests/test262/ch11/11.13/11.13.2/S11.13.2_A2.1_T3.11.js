










try {
  var z = (x |= 1);
  $ERROR('#1.1: x |= 1 throw ReferenceError. Actual: ' + (z));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: x |= 1 throw ReferenceError. Actual: ' + (e));  
  }
}

