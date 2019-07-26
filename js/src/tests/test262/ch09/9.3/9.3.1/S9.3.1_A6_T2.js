










function dynaString(s1, s2){
  return String(s1)+String(s2);
}



if (Number(dynaString("Infi", "nity")) !== Number.POSITIVE_INFINITY) {
  $ERROR('#1: Number("Infi"+"nity") === Number.POSITIVE_INFINITY');
}


if (Number(dynaString("Infi", "nity")) !== 10e10000) {
  $ERROR('#2: Number("Infi"+"nity") === 10e10000');
}


if (Number(dynaString("Infi", "nity")) !== 10E10000) {
  $ERROR('#3: Number("Infi"+"nity") === 10E10000');
}


if (Number(dynaString("Infi", "nity")) !== Number("10e10000")) {
  $ERROR('#4: Number("Infi"+"nity") === Number("10e10000")');
}

