










if (-1e+308*3 !== -Infinity){
  $ERROR('#1: -1e+308*3 === Infinity. Actual: ' + (-1e+308*3));
}


if ((-1*(Math.pow(2,53))*(Math.pow(2,971))) !== -Infinity){
  $ERROR('#2: (-1*(Math.pow(2,53))*(Math.pow(2,971))) === Infinity. Actual: ' + ((-1*(Math.pow(2,53))*(Math.pow(2,971)))));
}

