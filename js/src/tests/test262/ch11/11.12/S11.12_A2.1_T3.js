










try {
  true ? y : false;
  $ERROR('#1.1: true ? y : false throw ReferenceError. Actual: ' + (true ? y : false));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: true ? y : false throw ReferenceError. Actual: ' + (e));  
  }
}

