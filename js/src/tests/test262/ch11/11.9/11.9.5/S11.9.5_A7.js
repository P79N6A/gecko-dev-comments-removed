











if (!(new Object() !== new Object())) {
  $ERROR('#1: new Object() !== new Object()');
}


if (!(new Object(true) !== new Object(true))) {
  $ERROR('#2: new Object() !== new Object()');
}


if (!(new Object(false) !== new Object(false))) {
  $ERROR('#3: new Object() !== new Object()');
}


if (!(new Object(+0) !== new Object(-0))) {
  $ERROR('#4: new Object(+0) !== new Object(-0)');
}


x = {}; 
y = x;
if (x !== y) {
  $ERROR('#5: x = {}; y = x; x === y');
}


if (!(new Boolean(true) !== new Number(1))) {
  $ERROR('#6 new Boolean(true) !== new Number(1)');
}


if (!(new Number(1) !== new String("1"))) {
  $ERROR('#7: new Number(1) !== new String("1")');
}


if (!(new String("1") !== new Boolean(true))) {
  $ERROR('#8: new String("x") !== new Boolean(true)');
}



