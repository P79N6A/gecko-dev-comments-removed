











if ((new Boolean(true) == new Boolean(true)) !== false) {
  $ERROR('#1: (new Boolean(true) == new Boolean(true)) === false');
}


if ((new Number(1) == new Number(1)) !== false) {
  $ERROR('#2: (new Number(1) == new Number(1)) === false');
}


if ((new String("x") == new String("x")) !== false) {
  $ERROR('#3: (new String("x") == new String("x")) === false');
}


if ((new Object() == new Object()) !== false) {
  $ERROR('#4: (new Object() == new Object()) === false');
}


x = {}; 
y = x;
if ((x == y) !== true) {
  $ERROR('#5: x = {}; y = x; (x == y) === true');
}


if ((new Boolean(true) == new Number(1)) !== false) {
  $ERROR('#6 (new Boolean(true) == new Number(1)) === false');
}


if ((new Number(1) == new String("1")) !== false) {
  $ERROR('#7: (new Number(1) == new String("1")) === false');
}


if ((new String("1") == new Boolean(true)) !== false) {
  $ERROR('#8: (new String("x") == new Boolean(true)) === false');
}

