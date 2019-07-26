









var p_zero=+0;
var n_zero=-0;


if ((p_zero == n_zero) !== true){
  $ERROR('#1: var p_zero=+0; var n_zero=-0; p_zero != n_zero');
}


if ((n_zero == 0) !== true){
  $ERROR('#2: var p_zero=+0; var n_zero=-0; n_zero == 0');
}


if ((p_zero == -0) !== true){
  $ERROR('#3: var p_zero=+0; var n_zero=-0; p_zero == -0');
}


if ((p_zero === 0) !== true){
  $ERROR('#4: var p_zero=+0; var n_zero=-0; p_zero === 0');
}


if ((n_zero === -0) !== true){
  $ERROR('#5: var p_zero=+0; var n_zero=-0; n_zero === -0');
}

