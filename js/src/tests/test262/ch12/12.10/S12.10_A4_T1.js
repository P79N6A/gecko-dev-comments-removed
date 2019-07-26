










this.p1 = 1;
var myObj = {
  p1: 'a', 
}
eval("with(myObj){p1='b'}");



if(myObj.p1 !== 'b'){
  $ERROR('#1: myObj.p1 === "b". Actual:  myObj.p1 ==='+ myObj.p1  );
}





if(myObj.p1 === 1){
  $ERROR('#2: myObj.p1 !== 1');
}



