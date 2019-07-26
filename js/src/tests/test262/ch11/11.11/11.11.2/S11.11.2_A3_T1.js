










if ((false || true) !== true) {
  $ERROR('#1: (false || true) === true');
}


if ((false || false) !== false) {
  $ERROR('#2: (false || false) === false');
}


var y = new Boolean(true);
if ((false || y) !== y) {
  $ERROR('#3: (var y = new Boolean(true); false || y) === y');
}


var y = new Boolean(false);
if ((false || y) !== y) {
  $ERROR('#4: (var y = new Boolean(false); false || y) === y');
}

