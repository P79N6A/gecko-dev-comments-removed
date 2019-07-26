










try {
  new x;
  $ERROR('#1.1: new x throw ReferenceError. Actual: ' + (new x));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: new x throw ReferenceError. Actual: ' + (e));  
  }
}


try {
  new x();
  $ERROR('#2: new x() throw ReferenceError'); 
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#2: new x() throw ReferenceError'); 
  }
}

