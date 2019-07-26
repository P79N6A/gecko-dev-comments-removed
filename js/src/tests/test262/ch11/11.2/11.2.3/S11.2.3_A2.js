










try {
  x();
  $ERROR('#1.1: x() throw ReferenceError. Actual: ' + (x()));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: x() throw ReferenceError. Actual: ' + (e));  
  }
}


try {
  x(1,2,3);
  $ERROR('#2.1: x(1,2,3) throw ReferenceError. Actual: ' + (x(1,2,3))); 
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#2.2: x(1,2,3) throw ReferenceError. Actual: ' + (e)); 
  }
}

