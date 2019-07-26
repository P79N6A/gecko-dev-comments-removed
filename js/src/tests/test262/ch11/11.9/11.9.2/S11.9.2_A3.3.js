











if ((0 != false) !== false) {
  $ERROR('#1: (0 != false) === false');
}


if (("1" != true) !== false) {
  $ERROR('#2: ("1" != true) === false');
}


if ((new Boolean(false) != false) !== false) {
  $ERROR('#3: (new Boolean(false) != false) === false');
}


if (({valueOf: function () {return "0"}} != false) !== false) {
  $ERROR('#4: ({valueOf: function () {return "0"}} != false) === false');
}

