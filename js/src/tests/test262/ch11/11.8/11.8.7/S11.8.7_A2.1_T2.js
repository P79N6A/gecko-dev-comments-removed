










try {
  MAX_VALUE in Number;
  $ERROR('#1.1: MAX_VALUE in Number throw ReferenceError. Actual: ' + (MAX_VALUE in Number));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: MAX_VALUE in Number throw ReferenceError. Actual: ' + (e));  
  }
}

