










try {
  object instanceof (object = {}, Object);
  $ERROR('#1.1: object instanceof (object = {}, Object) throw ReferenceError. Actual: ' + (object instanceof (object = {}, Object)));  
}
catch (e) {
  if ((e instanceof ReferenceError) !== true) {
    $ERROR('#1.2: object instanceof (object = {}, Object) throw ReferenceError. Actual: ' + (e));  
  }
}


if ((OBJECT = Object, {}) instanceof OBJECT !== true) {
  $ERROR('#2: (OBJECT = Object, {}) instanceof OBJECT !== true');
}


