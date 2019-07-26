










if ((true ? false : true) !== false) {
  $ERROR('#1: (true ? false : true) === false');
}


var y = new Boolean(true);
if ((true ? y : false) !== y) {
  $ERROR('#2: (var y = new Boolean(true); (true ? y : false) === y');
}


var y = new Boolean(false);
if ((y ? y : true) !== y) {
  $ERROR('#3: (var y = new Boolean(false); (y ? y : true) === y');
}

