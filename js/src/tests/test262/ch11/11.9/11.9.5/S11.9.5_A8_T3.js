










if (!("1" !== new String("1"))) {
  $ERROR('#1: "1" !== new String("1")');
}


if (!("1" !== true)) {
  $ERROR('#2: "1" !== true');
}


if (!("1" !== new Boolean("1"))) {
  $ERROR('#3: "1" !== new Boolean("1")');
}


if (!("1" !== 1)) {
  $ERROR('#4: "1" === 1');
}


if (!("1" !== new Number("1"))) {
  $ERROR('#5: "1" === new Number("1")');
}


if (!(new String(false) !== false)) {
  $ERROR('#6: new Number(false) !== false');
}


if (!(false !== "0")) {
  $ERROR('#7: false !== "0"');
}


if (!("0" !== new Boolean("0"))) {
  $ERROR('#8: "0" !== new Boolean("0")');
}


if (!(false !== 0)) {
  $ERROR('#9: false !== 0');
}


if (!(false !== new Number(false))) {
  $ERROR('#10: false !== new Number(false)');
}


if (!("1" !== {valueOf: function () {return "1"}})) {
  $ERROR('#11: "1" !== {valueOf: function () {return "1"}}');
}


