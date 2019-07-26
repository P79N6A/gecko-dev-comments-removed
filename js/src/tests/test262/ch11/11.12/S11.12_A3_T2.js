










if ((0 ? 0 : 1) !== 1) {
  $ERROR('#1: (0 ? 0 : 1) === 1');
}


var z = new Number(1);
if ((0 ? 1 : z) !== z) {
  $ERROR('#2: (var y = new Number(1); (0 ? 1 : z) === z');
}

