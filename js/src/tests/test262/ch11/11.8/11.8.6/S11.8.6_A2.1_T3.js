










try {
  ({}) instanceof OBJECT;
  $ERROR('#1.1: ({}) instanceof OBJECT throw ReferenceError. Actual: ' + (({}) instanceof OBJECT));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: ({}) instanceof OBJECT throw ReferenceError. Actual: ' + (e));  
  }
}

