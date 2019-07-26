










try {
  new this;
  $ERROR('#1: new this throw TypeError');	
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1: new this throw TypeError');	
  }
}


try {
  new this();
  $ERROR('#2: new this() throw TypeError'); 
}
catch (e) {
  if ((e instanceof TypeError) !== true) {
    $ERROR('#2: new this() throw TypeError'); 
  }
}

