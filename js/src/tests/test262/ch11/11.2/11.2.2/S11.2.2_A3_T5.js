










try {
  new null;
  $ERROR('#1: new null throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1: new null throw TypeError');	
  }
}


try {
  var x = null;
  new x;
  $ERROR('#2: var x = null; new x throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2: var x = null; new x throw TypeError');	
  }
}


try {
  var x = null;
  new x();
  $ERROR('#3: var x = null; new x() throw TypeError'); 
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#3: var x = null; new x() throw TypeError'); 
  }
}

