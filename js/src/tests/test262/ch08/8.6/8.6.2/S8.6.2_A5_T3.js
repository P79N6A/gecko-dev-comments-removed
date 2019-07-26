










var count=0;
var knock=function(){count++};


knock();
if (count !==1) {
  $ERROR('#1: count=0; knock=function(){count++}; knock(); count === 1. Actual: ' + (count));
}





this['knock']();
if (count !==2) {
  $ERROR('#2: count=0; knock=function(){count++}; knock(); this[\'knock\'](); count === 2. Actual: ' + (count));
}



