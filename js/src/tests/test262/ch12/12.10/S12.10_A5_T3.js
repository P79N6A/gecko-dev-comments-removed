










this.p1 = 'a';
var myObj = {
  p1: true,
  del:false 
}

eval("with(myObj){del = delete p1}");



if(myObj.p1 === true){
  $ERROR('#1: myObj.p1 !== true ');
}





if(myObj.p1 !== undefined){
  $ERROR('#2: myObj.p1 === undefined . Actual:  myObj.p1 ==='+ myObj.p1  );
}





if(myObj.del !== true){
  $ERROR('#3: myObj.del === true . Actual:  myObj.del ==='+ myObj.del  );
}





if(myObj.p1 === 'a'){
  $ERROR('#4: myObj.p1 !== \'a\'');
}



