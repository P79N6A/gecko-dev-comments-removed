










var c=0, i=0;
var fin=0;
do{
  i+=1;
  try{
    if(c===0){
      throw "ex1";
      $ERROR('#1.1: throw "ex1" lead to throwing exception');
    }
    c+=2;
    if(c===1){
      throw "ex2";
      $ERROR('#1.2: throw "ex2" lead to throwing exception');
    }
  }
  catch(er1){
    c-=1;
    continue;
    $ERROR('#1.3: "try catch{continue} finally" must work correctly');
  }
  finally{
    fin+=1;
  }
}
while(i<10);
if(fin!==10){
  $ERROR('#1.4: "finally" block must be evaluated');
}


