










try {
  false || y;
  $ERROR('#1.1: false || y throw ReferenceError. Actual: ' + (false || y));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: false || y throw ReferenceError. Actual: ' + (e));  
  }
}

