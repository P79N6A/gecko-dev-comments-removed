









var str='ABC';
var strObj=new String('ABC');



if (str.constructor !== strObj.constructor){
  $ERROR('#1: \'ABC\'.constructor === new String(\'ABC\').constructor');
}





if (str != strObj){
  $ERROR('#2: "ABC" == new String("ABC")');
}





if (str === strObj){
  $ERROR('#3: "ABC" !== new String("ABC")');
}



