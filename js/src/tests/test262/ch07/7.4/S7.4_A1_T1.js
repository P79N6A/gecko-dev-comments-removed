













var x = 0;

if (x !== 0) {
  $ERROR('#2: var x = 0; // x = 1; x === 0. Actual: ' + (x));
}


var 
y;
if (y !== undefined) {
  $ERROR('#3: var // y = 1; \\n y; y === undefined. Actual: ' + (y));
}  












this.y++;
if (isNaN(y) !== true) {
  $ERROR('#6: //var this.y = 1; \\n this.y++; y === Not-a-Number. Actual: ' + (y));
}


