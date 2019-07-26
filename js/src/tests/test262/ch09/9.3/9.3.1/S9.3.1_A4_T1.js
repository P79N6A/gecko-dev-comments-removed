










if (Number("+0") !== Number("0")) {
  $ERROR('#1.1: Number("+0") === Number("0")');
} else {
  
  if (1/Number("+0") !== 1/Number("0")) {
    $ERROR('#2.2: 1/Number("+0") === 1/Number("0")');
  }
}


if (Number("+Infinity") !== Number("Infinity")) {
  $ERROR('#3: Number("+Infinity") === Number("Infinity")');
}


if (Number("+1234.5678") !== Number("1234.5678")) {
  $ERROR('#4: Number("+1234.5678") === Number("1234.5678")');
}


if (Number("+1234.5678e90") !== Number("1234.5678e90")) {
  $ERROR('#5: Number("+1234.5678e90") === Number("1234.5678e90")');
}


if (Number("+1234.5678E90") !== Number("1234.5678E90")) {
  $ERROR('#6: Number("+1234.5678E90") === Number("1234.5678E90")');
}


if (Number("+1234.5678e-90") !== Number("1234.5678e-90")) {
  $ERROR('#7: Number("+1234.5678e-90") === Number("1234.5678e-90")');
}


if (Number("+1234.5678E-90") !== Number("1234.5678E-90")) {
  $ERROR('#8: Number("+1234.5678E-90") === Number("1234.5678E-90")');
}

