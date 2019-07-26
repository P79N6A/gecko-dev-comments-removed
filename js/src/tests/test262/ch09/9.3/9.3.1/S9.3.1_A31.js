










if (Number("0xf") !== 15)  {
  $ERROR('#1: Number("0xf") === 15. Actual: ' + (Number("0xf")));
}


if (Number("0xF") !== 15)  {
  $ERROR('#2: Number("0xF") === 15. Actual: ' + (Number("0xF")));
}


if (+("0Xf") !== 15)  {
  $ERROR('#3: +("0Xf") === 15. Actual: ' + (+("0Xf")));
}


if (Number("0XF") !== 15)  {
  $ERROR('#4: Number("0XF") === 15. Actual: ' + (Number("0XF")));
}

