










if (1 + 1 !== 2) {
  $ERROR('#1: 1 + 1 === 2. Actual: ' + (1 + 1));
}


var x = 1;
if (x + 1 !== 2) {
  $ERROR('#2: var x = 1; x + 1 === 2. Actual: ' + (x + 1));
}


var y = 1;
if (1 + y !== 2) {
  $ERROR('#3: var y = 1; 1 + y === 2. Actual: ' + (1 + y));
}


var x = 1;
var y = 1;
if (x + y !== 2) {
  $ERROR('#4: var x = 1; var y = 1; x + y === 2. Actual: ' + (x + y));
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = 1;
objecty.prop = 1;
if (objectx.prop + objecty.prop !== 2) {
  $ERROR('#5: var objectx = new Object(); var objecty = new Object(); objectx.prop = 1; objecty.prop = 1; objectx.prop + objecty.prop === 2. Actual: ' + (objectx.prop + objecty.prop));
}

