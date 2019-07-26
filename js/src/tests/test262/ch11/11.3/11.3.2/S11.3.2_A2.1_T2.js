










try {
  x--;
  $ERROR('#1.1: x-- throw ReferenceError. Actual: ' + (x--));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: x-- throw ReferenceError. Actual: ' + (e));  
  }
}

