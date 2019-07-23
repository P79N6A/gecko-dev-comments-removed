


















































var gTestfile = 'regress-108440.js';
var BUGNUMBER = 108440;
var summary = "Shouldn't crash trying to add an array as an element of itself";
var self = this;
var temp = '';

printBugNumber(BUGNUMBER);
printStatus(summary);




var a=[];
temp = (a[a.length]=a);




a=[];
for(var prop in self)
{
  temp = prop;
  temp = (a[a.length] = self[prop]);
}




a=[];
for (var i=0; i<10; i++)
{
  a[a.length] = a;
}




a=[];
for (var i=0; i<10; i++)
{
  a[a.length] = a.toString();
}




a=[];
try
{
  for (var i=0; i<10; i++)
  {
    a[a.length] = a.toSource();
  }
}
catch(e)
{
}

reportCompare('No Crash', 'No Crash', '');
