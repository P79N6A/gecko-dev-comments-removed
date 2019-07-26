









var str='';



if (str == undefined){
  $ERROR('#1: Empty string and undefined are not equal (!=) to each other');
}





if (str == null){
  $ERROR('#1: Empty string and Null are not equal (!=) to each other');
}





if (str != 0){
  $ERROR('#3: Empty string and 0 are equal (==) to each other, since they all evaluate to 0');
}





if (str != false){
  $ERROR('#4: Empty string and false are equal (==) to each other, since they all evaluate to 0');
}



