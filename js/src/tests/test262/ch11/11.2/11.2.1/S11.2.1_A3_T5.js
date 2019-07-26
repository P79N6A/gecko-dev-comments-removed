










try {
  null.toString();
  $ERROR('#1.1: null.toString() throw TypeError. Actual: ' + (null.toString()));  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: null.toString() throw TypeError. Actual: ' + (e));  
  }
}


try {  
  null["toString"]();
  $ERROR('#2.1: null["toString"]() throw TypeError. Actual: ' + (null["toString"]())); 
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2.2: null["toString"]() throw TypeError. Actual: ' + (e)); 
  }
}

