





































var gTestfile = 'regress-290592.js';

var BUGNUMBER = 290592;
var summary = 'Array extras: forEach, indexOf, filter, map';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);



function identity(v, index, array)
{
  reportCompare(v, array[index], 'identity: check callback argument consistency');
  return v;
} 

function mutate(v, index, array)
{
  reportCompare(v, array[index], 'mutate: check callback argument consistency');
  if (index == 0)
  {
    array[1] = 'mutated';
    delete array[2];
    array.push('not visited');
  }
  return v;
}

function mutateForEach(v, index, array)
{
  reportCompare(v, array[index], 'mutateForEach: check callback argument consistency');
  if (index == 0)
  {
    array[1] = 'mutated';
    delete array[2];
    array.push('not visited');
  }
  actual += v + ',';
}

function makeUpperCase(v, index, array)
{
  reportCompare(v, array[index], 'makeUpperCase: check callback argument consistency');
  try
  {
    return v.toUpperCase();
  }
  catch(e)
  {
  }
  return v;
}


function concat(v, index, array)
{
  reportCompare(v, array[index], 'concat: check callback argument consistency');
  actual += v + ',';
}


function isUpperCase(v, index, array)
{
  reportCompare(v, array[index], 'isUpperCase: check callback argument consistency');
  try
  {
    return v == v.toUpperCase();
  }
  catch(e)
  {
  }
  return false;
}

function isString(v, index, array)
{
  reportCompare(v, array[index], 'isString: check callback argument consistency');
  return typeof v == 'string';
}



function ArrayCallback(state)
{
  this.state = state;
}

ArrayCallback.prototype.makeUpperCase = function (v, index, array)
{
  reportCompare(v, array[index], 'ArrayCallback.prototype.makeUpperCase: check callback argument consistency');
  try
  {
    return this.state ? v.toUpperCase() : v.toLowerCase();
  }
  catch(e)
  {
  }
  return v;
};

ArrayCallback.prototype.concat = function(v, index, array)
{
  reportCompare(v, array[index], 'ArrayCallback.prototype.concat: check callback argument consistency');
  actual += v + ',';
};

ArrayCallback.prototype.isUpperCase = function(v, index, array)
{
  reportCompare(v, array[index], 'ArrayCallback.prototype.isUpperCase: check callback argument consistency');
  try
  {
    return this.state ? true : (v == v.toUpperCase());
  }
  catch(e)
  {
  }
  return false;
};

ArrayCallback.prototype.isString = function(v, index, array)
{
  reportCompare(v, array[index], 'ArrayCallback.prototype.isString: check callback argument consistency');
  return this.state ? true : (typeof v == 'string');
};

function dumpError(e)
{
  var s = e.name + ': ' + e.message +
    ' File: ' + e.fileName +
    ', Line: ' + e.lineNumber +
    ', Stack: ' + e.stack;
  return s;
}

var obj;
var strings = ['hello', 'Array', 'WORLD'];
var mixed   = [0, '0', 0];
var sparsestrings = new Array();
sparsestrings[2] = 'sparse';

if ('map' in Array.prototype)
{


  

  
  expect = 1;
  actual = Array.prototype.map.length;
  reportCompare(expect, actual, 'Array.prototype.map.length == 1');

  
  expect = 'TypeError';
  try
  {
    strings.map();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.map(undefined) throws TypeError'); 

  try
  {
    
    expect = 'hello,Array,WORLD';
    actual = strings.map(identity).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: identity'); 


  try
  {
    expect = 'hello,mutated,';
    actual = strings.map(mutate).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: mutate'); 

  strings = ['hello', 'Array', 'WORLD'];

  try
  {
    
    expect = 'HELLO,ARRAY,WORLD';
    actual = strings.map(makeUpperCase).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: uppercase'); 

  try
  {
    
    expect = 'HELLO,ARRAY,WORLD';
    var obj = new ArrayCallback(true);
    actual  = strings.map(obj.makeUpperCase, obj).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: uppercase with object callback'); 

  try
  {
    expect = 'hello,array,world';
    obj = new ArrayCallback(false);
    actual = strings.map(obj.makeUpperCase, obj).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: lowercase with object callback'); 

  try
  {
    
    expect = ',,SPARSE';
    actual = sparsestrings.map(makeUpperCase).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.map: uppercase on sparse array'); 
}

if ('forEach' in Array.prototype)
{


  

  
  expect = 1;
  actual = Array.prototype.forEach.length;
  reportCompare(expect, actual, 'Array.prototype.forEach.length == 1');

  
  expect = 'TypeError';
  try
  {
    strings.forEach();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.forEach(undefined) throws TypeError'); 

  try
  {
    
    expect = 'hello,Array,WORLD,';
    actual = '';
    strings.forEach(concat);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach'); 

  try
  {
    expect = 'hello,mutated,';
    actual = '';
    strings.forEach(mutateForEach);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach: mutate'); 

  strings = ['hello', 'Array', 'WORLD'];



  try
  {
    
    expect = 'hello,Array,WORLD,';
    actual = '';
    obj = new ArrayCallback(true);
    strings.forEach(obj.concat, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach with object callback 1'); 

  try
  {
    expect = 'hello,Array,WORLD,';
    actual = '';
    obj = new ArrayCallback(false);
    strings.forEach(obj.concat, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach with object callback 2'); 

  try
  {
    
    
    expect = 'sparse,';
    actual = '';
    sparsestrings.forEach(concat);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.forEach on sparse array'); 
}

if ('filter' in Array.prototype)
{


  

  
  expect = 1;
  actual = Array.prototype.filter.length;
  reportCompare(expect, actual, 'Array.prototype.filter.length == 1');

  
  expect = 'TypeError';
  try
  {
    strings.filter();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.filter(undefined) throws TypeError'); 

  try
  {
    
    expect = 'WORLD';
    actual = strings.filter(isUpperCase).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.filter');

  try
  {
    expect = 'WORLD';
    obj = new ArrayCallback(false);
    actual = strings.filter(obj.isUpperCase, obj).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.filter object callback 1');

  try
  {
    expect = 'hello,Array,WORLD';
    obj = new ArrayCallback(true);
    actual = strings.filter(obj.isUpperCase, obj).toString();
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'Array.filter object callback 2');
}

if ('every' in Array.prototype)
{


  

  

  expect = 1;
  actual = Array.prototype.every.length;
  reportCompare(expect, actual, 'Array.prototype.every.length == 1');

  
  expect = 'TypeError';
  try
  {
    strings.every();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.every(undefined) throws TypeError'); 

  

  try
  {
    expect = true;
    actual = strings.every(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'strings: every element is a string');

  try
  {
    expect = false;
    actual = mixed.every(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'mixed: every element is a string');

  try
  {
    
    expect = true;
    actual = sparsestrings.every(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'sparsestrings: every element is a string');

  

  obj = new ArrayCallback(false);

  try
  {
    expect = true;
    actual = strings.every(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'strings: every element is a string, via object callback');

  try
  {
    expect = false;
    actual = mixed.every(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e) ;
  }
  reportCompare(expect, actual, 'mixed: every element is a string, via object callback');

  try
  {
    
    expect = true;
    actual = sparsestrings.every(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'sparsestrings: every element is a string, via object callback');

}

if ('some' in Array.prototype)
{


  

  

  expect = 1;
  actual = Array.prototype.some.length;
  reportCompare(expect, actual, 'Array.prototype.some.length == 1');

  
  expect = 'TypeError';
  try
  {
    strings.some();
    actual = 'no error';
  }
  catch(e)
  {
    actual = e.name;
  }
  reportCompare(expect, actual, 'Array.some(undefined) throws TypeError'); 

  

  try
  {
    expect = true;
    actual = strings.some(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'strings: some element is a string');

  try
  {
    expect = true;
    actual = mixed.some(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'mixed: some element is a string');

  try
  {
    expect = true;
    actual = sparsestrings.some(isString);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'sparsestrings: some element is a string');

  

  obj = new ArrayCallback(false);

  try
  {
    expect = true;
    actual = strings.some(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'strings: some element is a string, via object callback');

  try
  {
    expect = true;
    actual = mixed.some(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'mixed: some element is a string, via object callback');

  try
  {
    expect = true;
    actual = sparsestrings.some(obj.isString, obj);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'sparsestrings: some element is a string, via object callback');

}

if ('indexOf' in Array.prototype)
{


  

  

  expect = 1;
  actual = Array.prototype.indexOf.length;
  reportCompare(expect, actual, 'Array.prototype.indexOf.length == 1');

  

  try
  {
    expect = -1;
    actual = mixed.indexOf('not found');
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'indexOf returns -1 if value not found');

  try
  {
    expect = 0;
    actual = mixed.indexOf(0);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'indexOf matches using strict equality');

  try
  {
    expect = 1;
    actual = mixed.indexOf('0');
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'indexOf matches using strict equality');

  try
  {
    expect = 2;
    actual = mixed.indexOf(0, 1);
  }
  catch(e)
  {
    actual = dumpError(e);
  }
  reportCompare(expect, actual, 'indexOf begins searching at fromIndex');
}

