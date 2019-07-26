










try {
  new new Boolean(true);
  $ERROR('#1: new new Boolean(true) throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1: new new Boolean(true) throw TypeError');	
  }
}


try {
  var x = new Boolean(true);
  new x;
  $ERROR('#2: var x = new Boolean(true); new x throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2: var x = new Boolean(true); new x throw TypeError');	
  }
}


try {
  var x = new Boolean(true);
  new x();
  $ERROR('#3: var x = new Boolean(true); new x() throw TypeError');  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#3: var x = new Boolean(true); new x() throw TypeError');  
  }
}


