










var y = 1;



if(delete y){
  $ERROR('#1: y = 1; (delete y) === false. Actual: ' + ((delete y)));
};





if (y !== 1) {
  $ERROR('#2: y = 1; delete y; y === 1. Actual: ' + (y));
}



