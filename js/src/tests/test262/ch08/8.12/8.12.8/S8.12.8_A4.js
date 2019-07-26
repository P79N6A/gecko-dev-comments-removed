










try
{
  var __obj = {valueOf:function(){return new Object;},toString: function() {return new Object();}}  
  Number(__obj);
  $ERROR('#1.1: var __obj = {valueOf:function(){return new Object;},toNumber: function() {return new Object();}}; Number(__obj) throw TypeError. Actual: ' + (Number(__obj)));
}
catch(e)
{
  if ((e instanceof TypeError) !== true) {
    $ERROR('#1.2: var __obj = {valueOf:function(){return new Object;},toNumber: function() {return new Object();}}; Number(__obj) throw TypeError. Actual: ' + (e));
  }  
}  

