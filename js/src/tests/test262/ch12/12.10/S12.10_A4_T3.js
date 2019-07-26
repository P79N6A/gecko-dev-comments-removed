










this.p1 = 'a';
var myObj = {
  p1: true, 
}
eval("with(myObj){p1=false}");



if(myObj.p1 !== false){
  $ERROR('#1: myObj.p1 === false. Actual:  myObj.p1 ==='+ myObj.p1  );
}





if(myObj.p1 === 'a'){
  $ERROR('#2: myObj.p1 !== \'a\'');
}



