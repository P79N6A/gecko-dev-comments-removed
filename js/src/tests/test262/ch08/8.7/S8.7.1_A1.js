









this.y = 1;


if((delete this.y) !== true){
  $ERROR('#1: this.y = 1; (delete this.y) === true. Actual: ' + ((delete this.y)));
};






if (this.y !== undefined){
  $ERROR('#2: this.y = 1; (delete this.y) === true; this.y === undefined. Actual: ' + (this.y));
}



