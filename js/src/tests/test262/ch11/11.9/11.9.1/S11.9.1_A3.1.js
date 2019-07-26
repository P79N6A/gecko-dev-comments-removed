










if ((true == true) !== true) {
  $ERROR('#1: (true == true) === true');
}


if ((false == false) !== true) {
  $ERROR('#2: (false == false) === true');
}


if ((true == false) !== false) {
  $ERROR('#3: (true == false) === false');
}


if ((false == true) !== false) {
  $ERROR('#4: (false == true) === false');
}

