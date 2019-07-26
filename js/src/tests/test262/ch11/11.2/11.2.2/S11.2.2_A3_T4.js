










try {
  new undefined;
  $ERROR('#1: new undefined throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1: new undefined throw TypeError');	
  }
}


try {
  var x = undefined;
  new x;
  $ERROR('#2: var x = undefined; new x throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2: var x = undefined; new x throw TypeError');	
  }
}


try {
  var x = undefined;
  new x();
  $ERROR('#3: var x = undefined; new x() throw TypeError'); 
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#3: var x = undefined; new x() throw TypeError'); 
  }
}

