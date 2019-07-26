










try {
  true && y;
  $ERROR('#1.1: true && y throw ReferenceError. Actual: ' + (true && y));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: true && y throw ReferenceError. Actual: ' + (e));  
  }
}

