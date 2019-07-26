










if ((true || false) !== true) {
  $ERROR('#1: (true || false) === true');
}


if ((false || true) !== true) {
  $ERROR('#2: (false || true) === true');
}


var x = new Boolean(false);
if ((x || true) !== x) {
  $ERROR('#3: var x = Boolean(false); (x || true) === x');
}


var y = new Boolean(true);
if ((false || y) !== y) {
  $ERROR('#4: var y = Boolean(true); (false || y) === y');
}


var x = new Boolean(false);
var y = new Boolean(true);
if ((x || y) !== x) {
  $ERROR('#5: var x = new Boolean(false); var y = new Boolean(true); (x || y) === x');
}


var x = false;
var y = new Boolean(true);
if ((x || y) !== y) {
  $ERROR('#6: var x = false; var y = new Boolean(true); (x || y) === y');
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = false;
objecty.prop = 1.1;
if ((objectx.prop || objecty.prop) !== objecty.prop) {
  $ERROR('#7: var objectx = new Object(); var objecty = new Object(); objectx.prop = false; objecty.prop = 1; (objectx.prop || objecty.prop) === objecty.prop');
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = 1.1;
objecty.prop = false;
if ((objectx.prop || objecty.prop) !== objectx.prop) {
  $ERROR('#8: var objectx = new Object(); var objecty = new Object(); objectx.prop = 1.1; objecty.prop = false; (objectx.prop || objecty.prop) === objectx.prop');
}

