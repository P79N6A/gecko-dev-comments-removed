










x = 1;
if (x !== 1) {
  $ERROR('#1: x = 1; x === 1. Actual: ' + (x));
}


var x = 1;
if (x !== 1) {
  $ERROR('#2: var x = 1; x === 1. Actual: ' + (x));
}


y = 1;
x = y;
if (x !== 1) {
  $ERROR('#3: y = 1; x = y; x === 1. Actual: ' + (x));
}


var y = 1;
var x = y;
if (x !== 1) {
  $ERROR('#4: var y = 1; var x = y; x === 1. Actual: ' + (x));
}


var objectx = new Object();
var objecty = new Object();
objecty.prop = 1.1;
objectx.prop = objecty.prop;
if (objectx.prop !== objecty.prop) {
  $ERROR('#5: var objectx = new Object(); var objecty = new Object(); objecty.prop = 1; objectx.prop = objecty.prop; objectx.prop === objecty.prop. Actual: ' + (objectx.prop));
} else {
  if (objectx === objecty) {
    $ERROR('#5: var objectx = new Object(); var objecty = new Object(); objecty.prop = 1; objectx.prop = objecty.prop; objectx !== objecty');
  } 
}


