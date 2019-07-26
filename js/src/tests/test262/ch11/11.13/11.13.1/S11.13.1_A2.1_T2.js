










try {
  x = y;
  $ERROR('#1.1: x = y throw ReferenceError. Actual: ' + (x = y));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: x = y throw ReferenceError. Actual: ' + (e));  
  }
}

