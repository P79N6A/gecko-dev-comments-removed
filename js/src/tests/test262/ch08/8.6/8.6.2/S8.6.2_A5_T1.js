










this.count=0;

var testScreen = {touch:function(){count++}};


testScreen.touch();
if (count !==1) {
  $ERROR('#1: this.count=0; testScreen = {touch:function(){count++}}; testScreen.touch(); count === 1. Actual: ' + (count));
}





testScreen['touch']();
if (count !==2) {
  $ERROR('#2: this.count=0; testScreen = {touch:function(){count++}}; testScreen.touch(); testScreen[\'touch\'](); count === 2. Actual: ' + (count));
}



