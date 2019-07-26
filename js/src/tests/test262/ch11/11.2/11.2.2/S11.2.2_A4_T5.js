










try {
  new Math;
  $ERROR('#1: new Math throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1: new Math throw TypeError');	
  }
}


try {
  new new Math();
  $ERROR('#2: new new Math() throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2: new new Math() throw TypeError');	
  }
}


try {
  var x = new Math();
  new x();
  $ERROR('#3: var x = new Math(); new x() throw TypeError'); 
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#3: var x = new Math(); new x() throw TypeError'); 
  }
}


