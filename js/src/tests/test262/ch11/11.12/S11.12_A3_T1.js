










if ((false ? false : true) !== true) {
  $ERROR('#1: (false ? false : true) === true');
}


var z = new Boolean(true);
if ((false ? true : z) !== z) {
  $ERROR('#2: (var y = new Boolean(true); (false ? true : z) === z');
}

