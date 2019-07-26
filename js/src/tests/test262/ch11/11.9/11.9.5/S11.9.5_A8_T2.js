










if (!(1 !== new Number(1))) {
  $ERROR('#1: 1 !== new Number(1)');
}


if (!(1 !== true)) {
  $ERROR('#2: 1 !== true');
}


if (!(1 !== new Boolean(1))) {
  $ERROR('#3: 1 !== new Boolean(1)');
}


if (!(1 !== "1")) {
  $ERROR('#4: 1 !== "1"');
}


if (!(1 !== new String(1))) {
  $ERROR('#5: 1 !== new String(1)');
}


if (!(new Number(0) !== 0)) {
  $ERROR('#6: new Number(0) !== 0');
}


if (!(false !== 0)) {
  $ERROR('#7: false !== 0');
}


if (!(new Boolean(0) !== 0)) {
  $ERROR('#8: new Boolean(0) !== 0');
}


if (!("0" !== 0)) {
  $ERROR('#9: "0" !== 0');
}


if (!(new String(0) !== 0)) {
  $ERROR('#10: new String(0) !== 0');
}


if (!(1 !== {valueOf: function () {return 1}})) {
  $ERROR('#11: 1 !== {valueOf: function () {return 1}}');
}

