










if (!(undefined !== null)) {
  $ERROR('#1: undefined !== null');
}


if (!(null !== undefined)) {
  $ERROR('#2: null !== undefined');
}


if (!(null !== 0)) {
  $ERROR('#3: null !== 0');
}


if (!(0 !== null)) {
  $ERROR('#4: 0 !== null');
}


if (!(null !== false)) {
  $ERROR('#5: null !== false');
}


if (!(false !== null)) {
  $ERROR('#6: false !== null');
}


if (!(undefined !== false)) {
  $ERROR('#7: undefined !== false');
}


if (!(false !== undefined)) {
  $ERROR('#8: false !== undefined');
}


if (!(null !== new Object())) {
  $ERROR('#9: null !== new Object()');
}


if (!(new Object() !== null)) {
  $ERROR('#10: new Object() !== null');
}


if (!(null !== "null")) {
  $ERROR('#11: null !== "null"');
}


if (!("null" !== null)) {
  $ERROR('#12: "null" !== null');
}


if (!(undefined !== "undefined")) {
  $ERROR('#13: undefined !== "undefined"');
}


if (!("undefined" !== undefined)) {
  $ERROR('#14: "undefined" !== undefined');
}

