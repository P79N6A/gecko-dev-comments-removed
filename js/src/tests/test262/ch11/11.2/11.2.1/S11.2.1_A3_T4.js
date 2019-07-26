










try {
  undefined.toString();
  $ERROR('#1.1: undefined.toString() throw TypeError. Actual: ' + (undefined.toString()));  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: undefined.toString() throw TypeError. Actual: ' + (e));  
  }
}


try {  
  undefined["toString"]();
  $ERROR('#2.1: undefined["toString"]() throw TypeError. Actual: ' + (undefined["toString"]())); 
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2.2: undefined["toString"]() throw TypeError. Actual: ' + (e)); 
  }
}

