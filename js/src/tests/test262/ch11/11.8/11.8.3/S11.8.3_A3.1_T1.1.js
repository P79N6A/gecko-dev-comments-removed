










if (true <= true !== true) {
  $ERROR('#1: true <= true === true');
}


if (new Boolean(true) <= true !== true) {
  $ERROR('#2: new Boolean(true) <= true === true');
}


if (true <= new Boolean(true) !== true) {
  $ERROR('#3: true <= new Boolean(true) === true');
}


if (new Boolean(true) <= new Boolean(true) !== true) {
  $ERROR('#4: new Boolean(true) <= new Boolean(true) === true');
}

