










try {
  this();
  $ERROR('#1.1: this() throw TypeError. Actual: ' + (this()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: this() throw TypeError. Actual: ' + (e));	
  }
}

