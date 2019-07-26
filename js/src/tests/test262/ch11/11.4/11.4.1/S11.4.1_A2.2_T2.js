










function MyFunction(){}
var MyObject = new MyFunction();
if (delete MyObject.prop !== true) {
  $ERROR('#1: function MyFunction(){}; var MyObject = new MyFunction(); delete MyObject.prop === true');
}


var MyObject = new Object();
if (delete MyObject.prop !== true) {
  $ERROR('#2: var MyObject = new Object(); delete MyObject.prop === true');
}

