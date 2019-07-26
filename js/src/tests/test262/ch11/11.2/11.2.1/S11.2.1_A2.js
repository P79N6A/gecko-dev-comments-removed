










try {
  object[1];
  $ERROR('#1.1: object[1] throw ReferenceError. Actual: ' + (object[1]));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: object[1] throw ReferenceError. Actual: ' + (e));  
  }
}


try {
  object.prop;
  $ERROR('#2.1: object.prop throw ReferenceError. Actual: ' + (object.prop)); 
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#2.2: object.prop throw ReferenceError. Actual: ' + (e)); 
  }
}

