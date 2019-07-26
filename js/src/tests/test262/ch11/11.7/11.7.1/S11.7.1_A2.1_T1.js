










if (2 << 1 !== 4) {
  $ERROR('#1: 2 << 1 === 4. Actual: ' + (2 << 1));
}


var x = 2;
if (x << 1 !== 4) {
  $ERROR('#2: var x = 2; x << 1 === 4. Actual: ' + (x << 1));
}


var y = 1;
if (2 << y !== 4) {
  $ERROR('#3: var y = 2; 2 << y === 4. Actual: ' + (2 << y));
}


var x = 2;
var y = 1;
if (x << y !== 4) {
  $ERROR('#4: var x = 2; var y = 1; x << y === 4. Actual: ' + (x << y));
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = 2;
objecty.prop = 1;
if (objectx.prop << objecty.prop !== 4) {
  $ERROR('#5: var objectx = new Object(); var objecty = new Object(); objectx.prop = 2; objecty.prop = 1; objectx.prop << objecty.prop === 4. Actual: ' + (objectx.prop << objecty.prop));
}

