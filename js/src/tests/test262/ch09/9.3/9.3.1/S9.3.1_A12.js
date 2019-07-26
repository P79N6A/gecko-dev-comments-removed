












if (Number("12345e6") !== +("12345")*1e6)  {
  $ERROR('#1: Number("12345e6") === +("12345")*1e6');
}


if (Number("12345e-6") !== Number("12345")*1e-6)  {
  $ERROR('#2: Number("12345e-6") === Number("12345")*1e-6');
}

