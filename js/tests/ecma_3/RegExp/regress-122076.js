





















































var gTestfile = 'regress-122076.js';
var BUGNUMBER = 122076;
var summary = "Don't crash on invalid regexp literals /  \\/  /";
var STRESS = 10;
var sEval = '';

printBugNumber(BUGNUMBER);
printStatus(summary);


sEval += 'function checkDate()'
sEval += '{'
sEval += 'return (this.value.search(/^[012]?\d\/[0123]?\d\/[0]\d$/) != -1);'
sEval += '}'

sEval += 'function checkDNSName()'
sEval += '{'
sEval += '  return (this.value.search(/^([\w\-]+\.)+([\w\-]{2,3})$/) != -1);'
sEval += '}'

sEval += 'function checkEmail()'
sEval += '{'
sEval += '  return (this.value.search(/^([\w\-]+\.)*[\w\-]+@([\w\-]+\.)+([\w\-]{2,3})$/) != -1);'
sEval += '}'

sEval += 'function checkHostOrIP()'
sEval += '{'
sEval += '  if (this.value.search(/^([\w\-]+\.)+([\w\-]{2,3})$/) == -1)'
sEval += '    return (this.value.search(/^[1-2]?\d{1,2}\.[1-2]?\d{1,2}\.[1-2]?\d{1,2}\.[1-2]?\d{1,2}$/) != -1);'
sEval += '  else'
sEval += '    return true;'
sEval += '}'

sEval += 'function checkIPAddress()'
sEval += '{'
sEval += '  return (this.value.search(/^[1-2]?\d{1,2}\.[1-2]?\d{1,2}\.[1-2]?\d{1,2}\.[1-2]?\d{1,2}$/) != -1);'
sEval += '}'

sEval += 'function checkURL()'
sEval += '{'
sEval += '  return (this.value.search(/^(((https?)|(ftp)):\/\/([\-\w]+\.)+\w{2,4}(\/[%\-\w]+(\.\w{2,})?)*(([\w\-\.\?\\/\*\$+@&#;`~=%!]*)(\.\w{2,})?)*\/?)$/) != -1);'
sEval += '}'


for (var i=0; i<STRESS; i++)
{
  try
  {
    eval(sEval);
  }
  catch(e)
  {
  }
}

reportCompare('No Crash', 'No Crash', '');
