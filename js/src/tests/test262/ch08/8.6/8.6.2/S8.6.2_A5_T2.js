










this.position=0;
var seat = {};
seat['move']=function(){position++};


seat.move();
if (position !==1) {
  $ERROR('#1: this.position=0; seat = {}; seat[\'move\']=function(){position++}; seat.move(); position === 1. Actual: ' + (position));
}





seat['move']();
if (position !==2) {
  $ERROR('#2: this.position=0; seat = {}; seat[\'move\']=function(){position++}; seat.move(); seat[\'move\'](); position === 2. Actual: ' + (position));
}



