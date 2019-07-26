










if ((true ? false : true) !== false) {
  $ERROR('#1: (true ? false : true) === false');
}


if ((false ? false : true) !== true) {
  $ERROR('#2: (false ? false : true) === true');
}


var x = new Boolean(true);
var y = new Boolean(false);
if ((x ? y : true) !== y) {
  $ERROR('#3: var x = new Boolean(true); var y = new Boolean(false); (x ? y : true) === y');
}


var z = new Boolean(true);
if ((false ? false : z) !== z) {
  $ERROR('#4: var z = new Boolean(true); (false ? false : z) === z');
}


var x = new Boolean(true);
var y = new Boolean(false);
var z = new Boolean(true);
if ((x ? y : z) !== y) {
  $ERROR('#5: var x = new Boolean(true); var y = new Boolean(false); var z = new Boolean(true); (x ? y : z) === y');
}


var x = false;
var y = new Boolean(false);
var z = new Boolean(true);
if ((x ? y : z) !== z) {
  $ERROR('#6: var x = false; var y = new Boolean(false); var z = new Boolean(true); (x ? y : z) === z');
}

