










try {
  new true;
  $ERROR('#1: new true throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1: new true throw TypeError');	
  }
}


try {
  var x = true;
  new x;
  $ERROR('#2: var x = true; new x throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2: var x = true; new x throw TypeError');	
  }
}


try {
  var x = true;
  new x();
  $ERROR('#3: var x = true; new x() throw TypeError');  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#3: var x = true; new x() throw TypeError');  
  }
}


