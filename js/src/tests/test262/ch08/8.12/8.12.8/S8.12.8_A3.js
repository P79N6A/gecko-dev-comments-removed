










try
{
  var __obj = {toString: function() {return "1"}, valueOf: function() {return new Object();}}
  if (Number(__obj) !== 1) {
    $ERROR('#1.1: var __obj = {toNumber: function() {return "1"}, valueOf: function() {return new Object();}}; Number(__obj) === 1. Actual: ' + (Number(__obj)));
  }
}
catch(e)
{
  $ERROR('#1.2: var __obj = {toNumber: function() {return "1"}, valueOf: function() {return new Object();}}; Number(__obj) === 1. Actual: ' + (e));
}  



  


