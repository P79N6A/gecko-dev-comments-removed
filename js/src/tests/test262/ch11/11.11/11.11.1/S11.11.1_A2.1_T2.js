










try {
  x && true;
  $ERROR('#1.1: x && true throw ReferenceError. Actual: ' + (x && true));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: x && true throw ReferenceError. Actual: ' + (e));  
  }
}

