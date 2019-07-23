












































gTestsubsuite = 'extensions';

var MSG_PATTERN = '\nregexp = ';
var MSG_STRING = '\nstring = ';
var MSG_EXPECT = '\nExpect: ';
var MSG_ACTUAL = '\nActual: ';
var ERR_LENGTH = '\nERROR !!! match arrays have different lengths:';
var ERR_MATCH = '\nERROR !!! regexp failed to give expected match array:';
var ERR_NO_MATCH = '\nERROR !!! regexp FAILED to match anything !!!';
var ERR_UNEXP_MATCH = '\nERROR !!! regexp MATCHED when we expected it to fail !!!';
var CHAR_LBRACKET = '[';
var CHAR_RBRACKET = ']';
var CHAR_QT_DBL = '"';
var CHAR_QT = "'";
var CHAR_NL = '\n';
var CHAR_COMMA = ',';
var CHAR_SPACE = ' ';
var TYPE_STRING = typeof 'abc';



function testRegExp(statuses, patterns, strings, actualmatches, expectedmatches)
{
  var status = '';
  var pattern = new RegExp();
  var string = '';
  var actualmatch = new Array();
  var expectedmatch = new Array();
  var state = '';
  var lActual = -1;
  var lExpect = -1;


  for (var i=0; i != patterns.length; i++)
  {
    status = statuses[i];
    pattern = patterns[i];
    string = strings[i];
    actualmatch=actualmatches[i];
    expectedmatch=expectedmatches[i];
    state = getState(status, pattern, string);

    description = status;

    if(actualmatch)
    {
      actual = formatArray(actualmatch);
      if(expectedmatch)
      {
        
        lExpect = expectedmatch.length;
        lActual = actualmatch.length;

        var expected = formatArray(expectedmatch);

        if (lActual != lExpect)
        {
          reportCompare(lExpect, lActual,
                        state + ERR_LENGTH +
                        MSG_EXPECT + expected +
                        MSG_ACTUAL + actual +
                        CHAR_NL
	    );
          continue;
        }

        
        if (expected != actual)
        {
          reportCompare(expected, actual,
                        state + ERR_MATCH +
                        MSG_EXPECT + expected +
                        MSG_ACTUAL + actual +
                        CHAR_NL
	    );
        }
        else
        {
          reportCompare(expected, actual, state)
	    }

      }
      else 
      {
        expected = expectedmatch;
        reportCompare(expected, actual,
                      state + ERR_UNEXP_MATCH +
                      MSG_EXPECT + expectedmatch +
                      MSG_ACTUAL + actual +
                      CHAR_NL
	  );
      }

    }
    else 
    {
      if (expectedmatch)
      {
        actual = actualmatch;
        reportCompare(expected, actual,
                      state + ERR_NO_MATCH +
                      MSG_EXPECT + expectedmatch +
                      MSG_ACTUAL + actualmatch +
                      CHAR_NL
	  );
      }
      else 
      {
        
        expected = expectedmatch;
        actual   = actualmatch;
        reportCompare (expectedmatch, actualmatch, state);
      }
    }
  }
}


function getState(status, pattern, string)
{
  











  string = string.replace(/\n/g, '\\n');
  string = string.replace(/\r/g, '\\r');
  string = string.replace(/\t/g, '\\t');
  string = string.replace(/\v/g, '\\v');
  string = string.replace(/\f/g, '\\f');

  return (status + MSG_PATTERN + pattern + MSG_STRING + singleQuote(string));
}
















function formatArray(arr)
{
  try
  {
    return arr.toSource();
  }
  catch(e)
  {
    return toSource(arr);
  }
}











function toSource(arr)
{
  var delim = CHAR_COMMA + CHAR_SPACE;
  var elt = '';
  var ret = '';
  var len = arr.length;

  for (i=0; i<len; i++)
  {
    elt = arr[i];

    switch(true)
    {
    case (typeof elt === TYPE_STRING) :
      ret += doubleQuote(elt);
      break;

    case (elt === undefined || elt === null) :
      break; 

    default:
      ret += elt.toString();
    }

    if ((i < len-1) || (elt === undefined))
      ret += delim;
  }

  return  CHAR_LBRACKET + ret + CHAR_RBRACKET;
}


function doubleQuote(text)
{
  return CHAR_QT_DBL + text + CHAR_QT_DBL;
}


function singleQuote(text)
{
  return CHAR_QT + text + CHAR_QT;
}

