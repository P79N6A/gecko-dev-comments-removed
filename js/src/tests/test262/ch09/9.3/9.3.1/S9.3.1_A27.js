










if (Number("0xb") !== 11)  {
  $ERROR('#1: Number("0xb") === 11. Actual: ' + (Number("0xb")));
}


if (Number("0xB") !== 11)  {
  $ERROR('#2: Number("0xB") === 11. Actual: ' + (Number("0xB")));
}


if (+("0Xb") !== 11)  {
  $ERROR('#3: +("0Xb") === 11. Actual: ' + (+("0Xb")));
}


if (Number("0XB") !== 11)  {
  $ERROR('#4: Number("0XB") === 11. Actual: ' + (Number("0XB")));
}

