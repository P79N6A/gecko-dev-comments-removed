










this.p1 = 'a';
var myObj = {
  p1: function(){return 0;}, 
}
eval("with(myObj){p1=function(){return 1;}}");



if(myObj.p1() !== 1){
  $ERROR('#1: myObj.p1 === 1. Actual:  myObj.p1 ==='+ myObj.p1  );
}





if(myObj.p1 === 'a'){
  $ERROR('#2: myObj.p1 !== \'a\'');
}



