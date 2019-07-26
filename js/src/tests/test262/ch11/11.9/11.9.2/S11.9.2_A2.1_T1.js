










if ((1 != 1) !== false) {
  $ERROR('#1: (1 != 1) === false');
}


var x = 1;
if ((x != 1) !== false) {
  $ERROR('#2: var x = 1; (x != 1) === false');
}


var y = 1;
if ((1 != y) !== false) {
  $ERROR('#3: var y = 1; (1 != y) === false');
}


var x = 1;
var y = 1;
if ((x != y) !== false) {
  $ERROR('#4: var x = 1; var y = 1; (x != y) === false');
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = 1;
objecty.prop = 1;
if ((objectx.prop != objecty.prop) !== false) {
  $ERROR('#5: var objectx = new Object(); var objecty = new Object(); objectx.prop = 1; objecty.prop = 1; (objectx.prop != objecty.prop) === false');
}

