










var c1=0,fin=0;
while(c1<2){
  try{
    c1+=1;
    continue;
  }
  catch(er1){}
  finally{
    fin=1;
  }
  fin=-1;
};
if(fin!==1){
  $ERROR('#1: "finally" block must be evaluated at "try{continue} catch finally" construction');
}


var c2=0,fin2=0;
while(c2<2){
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
  $ERROR('#2: "finally" block must be evaluated at "try catch{continue} finally" construction');
}


var c3=0,fin3=0;
while(c3<2){
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
if(fin3!==1){
  $ERROR('#3: "finally" block must be evaluated at "try catch finally{continue}" construction');
}


var c4=0,fin4=0;
while(c4<2){
  try{
    c4+=1;
    continue;
  }
  finally{
    fin4=1;
  }
  fin4=-1;
};
if(fin4!==1){
  $ERROR('#4: "finally" block must be evaluated at "try{continue} finally" construction');
}


var c5=0;
while(c5<2){
  try{
    throw "ex1";
  }
  catch(er1){
    c5+=1;
    continue;
  }
}
if(c5!==2){
  $ERROR('#5: "try catch{continue}" must work correctly');
}


var c6=0,fin6=0;
while(c6<2){
  try{
    c6+=1;
    throw "ex1"
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
if(c6!==2){
  $ERROR('#6.2: "try finally{continue}" must work correctly');
}

