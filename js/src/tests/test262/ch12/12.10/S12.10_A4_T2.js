










this.p1 = 'a';
var myObj = {
  p1: 1, 
}
eval("with(myObj){p1=2}");



if(myObj.p1 !== 2){
  $ERROR('#1: myObj.p1 === 2. Actual:  myObj.p1 ==='+ myObj.p1  );
}





if(myObj.p1 === 'a'){
  $ERROR('#2: myObj.p1 !== \'a\'');
}



