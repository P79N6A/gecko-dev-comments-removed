










try {
  false ? true : z;
  $ERROR('#1.1: false ? true : z throw ReferenceError. Actual: ' + (false ? true : z));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: false ? true : z throw ReferenceError. Actual: ' + (e));  
  }
}

