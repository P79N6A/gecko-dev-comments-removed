










function f1()
{
  return 1;
}
if (f1() !== 1) { 
  $ERROR('#1: Check return statement for automatic semicolon insertion');
}  


function f2()
{
  return 
  1;
}
if (f2() !== undefined) { 
  $ERROR('#2: Check return statement for automatic semicolon insertion');
}  

