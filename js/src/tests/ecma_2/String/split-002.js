































var SECTION = "ecma_2/String/split-002.js";
var VERSION = "ECMA_2";
var TITLE   = "String.prototype.split( regexp, [,limit] )";

startTest();











CompareSplit( "hello", "ll" );

CompareSplit( "hello", "l" );
CompareSplit( "hello", "x" );
CompareSplit( "hello", "h" );
CompareSplit( "hello", "o" );
CompareSplit( "hello", "hello" );
CompareSplit( "hello", undefined );

CompareSplit( "hello", "");
CompareSplit( "hello", "hellothere" );

CompareSplit( new String("hello" ) );


Number.prototype.split = String.prototype.split;

CompareSplit( new Number(100111122133144155), 1 );
CompareSplitWithLimit(new Number(100111122133144155), 1, 1 );

CompareSplitWithLimit(new Number(100111122133144155), 1, 2 );
CompareSplitWithLimit(new Number(100111122133144155), 1, 0 );
CompareSplitWithLimit(new Number(100111122133144155), 1, 100 );
CompareSplitWithLimit(new Number(100111122133144155), 1, void 0 );
CompareSplitWithLimit(new Number(100111122133144155), 1, Math.pow(2,32)-1 );
CompareSplitWithLimit(new Number(100111122133144155), 1, "boo" );
CompareSplitWithLimit(new Number(100111122133144155), 1, -(Math.pow(2,32)-1) );
CompareSplitWithLimit( "hello", "l", NaN );
CompareSplitWithLimit( "hello", "l", 0 );
CompareSplitWithLimit( "hello", "l", 1 );
CompareSplitWithLimit( "hello", "l", 2 );
CompareSplitWithLimit( "hello", "l", 3 );
CompareSplitWithLimit( "hello", "l", 4 );






















test();

function CompareSplit( string, separator ) {
  split_1 = string.split( separator );
  split_2 = string_split( string, separator );

  AddTestCase(
    "( " + string +".split(" + separator + ") ).length" ,
    split_2.length,
    split_1.length );

  var limit = split_1.length > split_2.length ?
    split_1.length : split_2.length;

  for ( var split_item = 0; split_item < limit; split_item++ ) {
    AddTestCase(
      string + ".split(" + separator + ")["+split_item+"]",
      split_2[split_item],
      split_1[split_item] );
  }
}

function CompareSplitWithLimit( string, separator, splitlimit ) {
  split_1 = string.split( separator, splitlimit );
  split_2 = string_split( string, separator, splitlimit );

  AddTestCase(
    "( " + string +".split(" + separator + ", " + splitlimit+") ).length" ,
    split_2.length,
    split_1.length );

  var limit = split_1.length > split_2.length ?
    split_1.length : split_2.length;

  for ( var split_item = 0; split_item < limit; split_item++ ) {
    AddTestCase(
      string + ".split(" + separator  + ", " + splitlimit+")["+split_item+"]",
      split_2[split_item],
      split_1[split_item] );
  }
}

function string_split ( __this, separator, limit ) {
  var S = String(__this );					  

  var A = new Array();                          

  if ( limit == undefined ) {                   
    lim = Math.pow(2, 31 ) -1;
  } else {
    lim = ToUint32( limit );
  }

  var s = S.length;                              
  var p = 0;                                     

  if  ( separator == undefined ) {              
    A[0] = S;
    return A;
  }

  if ( separator.constructor == RegExp )         
    R = separator;
  else
    R = separator.toString();

  if (lim == 0) return A;                       

  if  ( separator == undefined ) {              
    A[0] = S;
    return A;
  }

  if (s == 0) {		                          
    z = SplitMatch(R, S, 0);
    if (z != false) return A;
    A[0] = S;
    return A;
  }

  var q = p;									  
loop:
  while (true ) {
	
    if ( q == s ) break;					  

    z = SplitMatch(R, S, q);                  



    if (z != false) {							
      e = z.endIndex;							
      cap = z.captures;						
      if (e != p) {							

	T = S.slice(p, q);					

	A[A.length] = T;					
	if (A.length == lim) return A;		
	p = e;								
	i = 0;								
	while (true) {						
	  if (i == cap.length) {              
	    q = p;                          
	    continue loop;
	  }
	  i = i + 1;							
	  A[A.length] = cap[i]				
	    if (A.length == lim) return A;		
	}
      }
    }

    q = q + 1;                               
  }

  T = S.slice(p, q);
  A[A.length] = T;
  return A;
}

function SplitMatch(R, S, q)
{
  if (R.constructor == RegExp) {			
    var reResult = R.match(S, q);		
    if (reResult == undefined)
      return false;
    else {
      a = new Array(reResult.length - 1);
      for (var i = 1; i < reResult.length; i++)
	a[a.length] = reResult[i];
      return { endIndex : reResult.index + reResult[0].length, captures : cap };
    }
  }
  else {
    var r = R.length;					
    s = S.length;						
    if ((q + r) > s) return false;		
    for (var i = 0; i < r; i++) {

      if (S.charAt(q + i) != R.charAt(i))			
	return false;
    }
    cap = new Array();								
    return { endIndex : q + r, captures : cap };	
  }
}

function ToUint32( n ) {
  n = Number( n );
  var sign = ( n < 0 ) ? -1 : 1;

  if ( Math.abs( n ) == 0
       || Math.abs( n ) == Number.POSITIVE_INFINITY
       || n != n) {
    return 0;
  }
  n = sign * Math.floor( Math.abs(n) )

    n = n % Math.pow(2,32);

  if ( n < 0 ){
    n += Math.pow(2,32);
  }

  return ( n );
}
