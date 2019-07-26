










if (1 % 2 !== 1) {
  $ERROR('#1: 1 % 2 === 1. Actual: ' + (1 % 2));
}


var x = 1;
if (x % 2 !== 1) {
  $ERROR('#2: var x = 1; x % 2 === 1. Actual: ' + (x % 2));
}


var y = 2;
if (1 % y !== 1) {
  $ERROR('#3: var y = 2; 1 % y === 1. Actual: ' + (1 % y));
}


var x = 1;
var y = 2;
if (x % y !== 1) {
  $ERROR('#4: var x = 1; var y = 2; x % y === 1. Actual: ' + (x % y));
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = 1;
objecty.prop = 2;
if (objectx.prop % objecty.prop !== 1) {
  $ERROR('#5: var objectx = new Object(); var objecty = new Object(); objectx.prop = 1; objecty.prop = 2; objectx.prop % objecty.prop === 1. Actual: ' + (objectx.prop % objecty.prop));
}

