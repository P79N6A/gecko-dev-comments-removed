












































var bug = 96128;
var summary = 'Testing that JS infinite recursion protection works';


function objRecurse()
{
  









  return new objRecurse();
}




test();



function test() 
{
  enterFunc ('test'); 
  printBugNumber (bug);
  printStatus (summary);

  
  var obj = new objRecurse();
 
  exitFunc ('test');
}
