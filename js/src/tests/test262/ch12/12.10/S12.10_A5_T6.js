










this.p1 = 'a';
var myObj = {
  p1: function(){return 0;}, 
  del:false
}
eval("with(myObj){del = delete p1}");



try{
if(myObj.p1() === 0){
  $ERROR('#1: myObj.p1() !== 0 ');
}
}catch(e){var x=1};
if(x !== 1){
  $ERROR('#1: x === 1. Actual:  x ==='+ x  );
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



