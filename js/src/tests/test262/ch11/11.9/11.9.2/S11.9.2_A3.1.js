










if ((true != true) !== false) {
  $ERROR('#1: (true != true) === false');
}


if ((false != false) !== false) {
  $ERROR('#2: (false != false) === false');
}


if ((true != false) !== true) {
  $ERROR('#3: (true != false) === true');
}


if ((false != true) !== true) {
  $ERROR('#4: (false != true) === true');
}

