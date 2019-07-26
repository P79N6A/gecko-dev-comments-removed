









var n = 1;
var m = n;

function addFirst2Second(first, second){first += second;}

addFirst2Second(n, m);



if (m !== 1) {
  $ERROR('#1: var n = 1; var m = n; function addFirst2Second(first, second){first += second;} addFirst2Second(n, m); m === 1. Actual: ' + (m));
}





