










if (+("0xd") !== 13)  {
  $ERROR('#1: +("0xd") === 13. Actual: ' + (+("0xd")));
}


if (Number("0xD") !== 13)  {
  $ERROR('#2: Number("0xD") === 13. Actual: ' + (Number("0xD")));
}


if (Number("0Xd") !== 13)  {
  $ERROR('#3: Number("0Xd") === 13. Actual: ' + (Number("0Xd")));
}


if (Number("0XD") !== 13)  {
  $ERROR('#4: Number("0XD") === 13. Actual: ' + (Number("0XD")));
}

