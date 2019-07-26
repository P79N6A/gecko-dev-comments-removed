










if ((false && true) !== false) {
  $ERROR('#1: (false && true) === false');
}


if ((true && false) !== false) {
  $ERROR('#2: (true && false) === false');
}


var x = false;
if ((x && true) !== false) {
  $ERROR('#3: var x = false; (x && true) === false');
}


var y = new Boolean(false);
if ((true && y) !== y) {
  $ERROR('#4: var y = new Boolean(false); (true && y) === y');
}


var x = false;
var y = true;
if ((x && y) !== false) {
  $ERROR('#5: var x = false; var y = true; (x && y) === false');
}


var x = true;
var y = new Boolean(false);
if ((x && y) !== y) {
  $ERROR('#6: var x = true; var y = new Boolean(false); (x && y) === y');
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = true;
objecty.prop = 1.1;
if ((objectx.prop && objecty.prop) !== objecty.prop) {
  $ERROR('#7: var objectx = new Object(); var objecty = new Object(); objectx.prop = true; objecty.prop = 1; (objectx.prop && objecty.prop) === objecty.prop');
}


var objectx = new Object();
var objecty = new Object();
objectx.prop = 0;
objecty.prop = true;
if ((objectx.prop && objecty.prop) !== objectx.prop) {
  $ERROR('#8: var objectx = new Object(); var objecty = new Object(); objectx.prop = 0; objecty.prop = true; (objectx.prop && objecty.prop) === objectx.prop');
}

