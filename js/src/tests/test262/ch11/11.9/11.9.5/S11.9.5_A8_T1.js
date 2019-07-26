










if (!(true !== new Boolean(true))) {
  $ERROR('#1: true !== new Number(true)');
}


if (!(true !== 1)) {
  $ERROR('#2: true !== 1');
}


if (!(true !== new Number(true))) {
  $ERROR('#3: true !== new Number(true)');
}


if (!(true !== "1")) {
  $ERROR('#4: true !== "1"');
}


if (!(true !== new String(true))) {
  $ERROR('#5: true !== new String(true)');
}


if (!(new Boolean(false) !== false)) {
  $ERROR('#6: new Number(false) !== false');
}


if (!(0 !== false)) {
  $ERROR('#7: 0 !== false');
}


if (!(new Number(false) !== false)) {
  $ERROR('#8: new Number(false) !== false');
}


if (!("0" !== false)) {
  $ERROR('#9: "0" !== false');
}


if (!(false !== new String(false))) {
  $ERROR('#10: false !== new String(false)');
}


if (!(true !== {valueOf: function () {return true}})) {
  $ERROR('#11: true !== {valueOf: function () {return true}}');
}

