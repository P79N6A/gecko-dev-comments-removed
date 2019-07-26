










try {
  Math();
  $ERROR('#1.1: Math() throw TypeError. Actual: ' + (Math()));	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: Math() throw TypeError. Actual: ' + (e));	
  }
}


