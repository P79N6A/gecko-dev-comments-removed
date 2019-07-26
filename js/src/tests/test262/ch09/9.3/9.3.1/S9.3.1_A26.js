










if (Number("0xa") !== 10)  {
  $ERROR('#1: Number("0xa") === 10. Actual: ' + (Number("0xa")));
}


if (Number("0xA") !== 10)  {
  $ERROR('#2: Number("0xA") === 10. Actual: ' + (Number("0xA")));
}


if (Number("0Xa") !== 10)  {
  $ERROR('#3: Number("0Xa") === 10. Actual: ' + (Number("0Xa")));
}


if (+("0XA") !== 10)  {
  $ERROR('#4: +("0XA") === 10. Actual: ' + (+("0XA")));
}

