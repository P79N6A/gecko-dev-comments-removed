










try {
  new new String("1");
  $ERROR('#1: new new String("1") throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1: new new String("1") throw TypeError');	
  }
}


try {
  var x = new String("1");
  new x;
  $ERROR('#2: var x = new String("1"); new x throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2: var x = new String("1"); new x throw TypeError');	
  }
}


try {
  var x = new String("1");
  new x();
  $ERROR('#3: var x = new String("1"); new x() throw TypeError');  
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#3: var x = new String("1"); new x() throw TypeError');  
  }
}


