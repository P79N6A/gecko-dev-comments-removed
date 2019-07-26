










var OBJECT = 0;
if ((OBJECT = Object, {}) instanceof OBJECT !== true) {
  $ERROR('#1: var OBJECT = 0; (OBJECT = Object, {}) instanceof OBJECT === true');
}


var object = {}; 
if (object instanceof (object = 0, Object) !== true) {
  $ERROR('#2: var object = {};  object instanceof (object = 0, Object) === true');
}


