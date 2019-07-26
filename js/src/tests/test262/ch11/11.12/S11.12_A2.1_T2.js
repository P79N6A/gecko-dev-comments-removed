










try {
  x ? true : false;
  $ERROR('#1.1: x ? true : false throw ReferenceError. Actual: ' + (x ? true : false));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: x ? true : false throw ReferenceError. Actual: ' + (e));  
  }
}

