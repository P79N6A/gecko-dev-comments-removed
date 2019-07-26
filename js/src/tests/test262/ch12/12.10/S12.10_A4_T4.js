










this.p1 = 'a';
var myObj = {
  p1: {a:"hello"}, 
}
eval("with(myObj){p1={b:'hi'}}");



if(myObj.p1.a === "hello"){
  $ERROR('#1: myObj.p1.a !== "hello"');
}





if(myObj.p1.b !== "hi"){
  $ERROR('#2: myObj.p1.b === "hi". Actual:  myObj.p1.b ==='+ myObj.p1.b  );
}






if(myObj.p1 === 'a'){
  $ERROR('#3: myObj.p1 !== \'a\'');
}



