












if ((true) !== true) {
  $ERROR('#1: (true) === true');
}


var x = new Boolean(true);
if ((x) !== x) {
  $ERROR('#2: var x = new Boolean(true); (x) === x. Actual: ' + ((x)));
}

