











if ((new Boolean(true) != new Boolean(true)) !== true) {
  $ERROR('#1: (new Boolean(true) != new Boolean(true)) === true');
}


if ((new Number(1) != new Number(1)) !== true) {
  $ERROR('#2: (new Number(1) != new Number(1)) === true');
}


if ((new String("x") != new String("x")) !== true) {
  $ERROR('#3: (new String("x") != new String("x")) === true');
}


if ((new Object() != new Object()) !== true) {
  $ERROR('#4: (new Object() != new Object()) === true');
}


x = {}; 
y = x;
if ((x != y) !== false) {
  $ERROR('#5: x = {}; y = x; (x != y) === false');
}


if ((new Boolean(true) != new Number(1)) !== true) {
  $ERROR('#6 (new Boolean(true) != new Number(1)) === true');
}


if ((new Number(1) != new String("1")) !== true) {
  $ERROR('#7: (new Number(1) != new String("1")) === true');
}


if ((new String("1") != new Boolean(true)) !== true) {
  $ERROR('#8: (new String("x") != new Boolean(true)) === true');
}

