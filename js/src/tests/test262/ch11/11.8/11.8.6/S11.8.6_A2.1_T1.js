










if (({}) instanceof Object !== true) {
  $ERROR('#1: ({}) instanceof Object === true');
}


var object = {};
if (object instanceof Object !== true) {
  $ERROR('#2: var object = {}; object instanceof Object === true');
}


var OBJECT = Object;
if (({}) instanceof OBJECT !== true) {
  $ERROR('#3: var OBJECT = Object; ({}) instanceof OBJECT === true');
}


var object = {};
var OBJECT = Object;
if (object instanceof OBJECT !== true) {
  $ERROR('#4: var object = {}; var OBJECT = Object; object instanceof OBJECT === true');
}


