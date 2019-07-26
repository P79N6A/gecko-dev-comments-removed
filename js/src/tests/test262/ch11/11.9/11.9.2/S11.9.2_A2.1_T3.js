










try {
  1 != y;
  $ERROR('#1: 1 != y throw ReferenceError');  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: 1 != y throw ReferenceError. Actual: ' + (e));  
  }
}


