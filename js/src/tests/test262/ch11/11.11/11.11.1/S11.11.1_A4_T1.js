










if ((true && true) !== true) {
  $ERROR('#1: (true && true) === true');
}


if ((true && false) !== false) {
  $ERROR('#2: (true && false) === false');
}


var y = new Boolean(true);
if ((new Boolean(true) &&  y) !== y) {
  $ERROR('#3: (var y = new Boolean(true); (new Boolean(true) &&  y) === y');
}


var y = new Boolean(false);
if ((new Boolean(true) &&  y) !== y) {
  $ERROR('#4: (var y = new Boolean(false); (new Boolean(true) &&  y) === y');
}


var y = new Boolean(true);
if ((new Boolean(false) &&  y) !== y) {
  $ERROR('#5: (var y = new Boolean(true); (new Boolean(false) &&  y) === y');
}


var y = new Boolean(false);
if ((new Boolean(false) &&  y) !== y) {
  $ERROR('#6: (var y = new Boolean(false); (new Boolean(false) &&  y) === y');
}

