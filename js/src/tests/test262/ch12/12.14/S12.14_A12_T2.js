









var x;
var mycars = new Array();
mycars[0] = "Saab";
mycars[1] = "Volvo";
mycars[2] = "BMW";


var fin=0;
var i=0;
for (x in mycars){
  try{
    i+=1;
    continue;
  }
  catch(er1){}
  finally{
    fin=1;
  }
  fin=-1;
}
if(fin!==1){
  $ERROR('#1.1: "finally" block must be evaluated');
}
if(i!==3){
  $ERROR('#1.2:  "try{continue} catch finally" must work correctly');
}


var c2=0,fin2=0;
for (x in mycars){
  try{
    throw "ex1";
  }
  catch(er1){
    c2+=1;
    continue;
  }
  finally{
    fin2=1;
  }
  fin2=-1;
}
if(fin2!==1){
  $ERROR('#2.1: "finally" block must be evaluated');
}
if(c2!==3){
  $ERROR('#2.1: "try catch{continue} finally" must work correctly');
}


var c3=0,fin3=0;
for (x in mycars){
  try{
    throw "ex1";
  }
  catch(er1){
    c3+=1;
  }
  finally{
    fin3=1;
    continue;
  }
  fin3=0;
}
if(c3!==3){
  $ERROR('#3.1: "finally" block must be evaluated');
}
if(fin3!==1){
  $ERROR('#3.2: "try catch finally{continue}" must work correctly');
}


var fin=0;
for (x in mycars){
  try{
    continue;
  }
  finally{
    fin=1;
  }
  fin=-1;
}
if(fin!==1){
  $ERROR('#4: "finally" block must be evaluated at "try{continue} finally" construction');
}


var c5=0;
for (x in mycars){
  try{
    throw "ex1";
  }
  catch(er1){
    c5+=1;
    continue;
  }
  c5+=12;
}
if(c5!==3){
  $ERROR('#5: "try catch{continue}" must work correctly');
}


var c6=0,fin6=0;
for (x in mycars){
  try{
    c6+=1;
    throw "ex1";
  }
  finally{
    fin6=1;
    continue;
  }
  fin6=-1;
}
if(fin6!==1){
  $ERROR('#6.1: "finally" block must be evaluated');
}
if(c6!==3){
  $ERROR('#6.2: "try finally{continue}" must work correctly');
}

