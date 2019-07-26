










x = 1;
if (delete x !== true) {
  $ERROR('#1: x = 1; delete x === true');
}


function MyFunction(){};
MyFunction.prop = 1;
if (delete MyFunction.prop !== true) {
  $ERROR('#2: function MyFunction(){}; MyFunction.prop = 1; delete MyFunction.prop === true');
}


function MyFunction(){};
var MyObject = new MyFunction();
MyObject.prop = 1;
if (delete MyObject.prop !== true) {
  $ERROR('#3: function MyFunction(){}; var MyObject = new MyFunction(); MyFunction.prop = 1; delete MyObject.prop === true');
}

