










if (Number("0xc") !== 12)  {
  $ERROR('#1: Number("0xc") === 12. Actual: ' + (Number("0xc")));
}


if (+("0xC") !== 12)  {
  $ERROR('#2: +("0xC") === 12. Actual: ' + (+("0xC")));
}


if (Number("0Xc") !== 12)  {
  $ERROR('#3: Number("0Xc") === 12. Actual: ' + (Number("0Xc")));
}


if (Number("0XC") !== 12)  {
  $ERROR('#4: Number("0XC") === 12. Actual: ' + (Number("0XC")));
}

