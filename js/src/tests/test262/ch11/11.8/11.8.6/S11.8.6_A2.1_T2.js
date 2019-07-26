










try {
  object instanceof Object;
  $ERROR('#1.1: object instanceof Object throw ReferenceError. Actual: ' + (object instanceof Object));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: object instanceof Object throw ReferenceError. Actual: ' + (e));  
  }
}

