









var p_zero=+0;
var n_zero=-0;

if (1.0/p_zero === 1.0/n_zero){
  $ERROR('#1: var p_zero=+0; var n_zero=-0; 1.0/p_zero !== 1.0/n_zero');
}

