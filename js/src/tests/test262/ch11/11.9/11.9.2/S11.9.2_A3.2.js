











if ((true != 1) !== false) {
  $ERROR('#1: (true != 1) === false');
}


if ((false != "0") !== false) {
  $ERROR('#2: (false != "0") === false');
}


if ((true != new Boolean(true)) !== false) {
  $ERROR('#3: (true != new Boolean(true)) === false');
}


if ((true != {valueOf: function () {return 1}}) !== false) {
  $ERROR('#4: (true != {valueOf: function () {return 1}}) === false');
}


