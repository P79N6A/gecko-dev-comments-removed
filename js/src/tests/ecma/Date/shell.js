
var BUGNUMBER;
var summary;

function runDSTOffsetCachingTestsFraction(part, parts)
{
  BUGNUMBER = 563938;
  summary = 'Cache DST offsets to improve SunSpider score';

  print(BUGNUMBER + ": " + summary);

  var MAX_UNIX_TIMET = 2145859200;
  var RANGE_EXPANSION_AMOUNT = 30 * 24 * 60 * 60;

  


  function tzOffsetFromUnixTimestamp(timestamp)
  {
    var d = new Date(NaN);
    d.setTime(timestamp); 
    return d.getTimezoneOffset(); 
  }

  





  function clearDSTOffsetCache(undesiredTimestamp)
  {
    var opposite = (undesiredTimestamp + MAX_UNIX_TIMET / 2) % MAX_UNIX_TIMET;

    
    tzOffsetFromUnixTimestamp(0);
    tzOffsetFromUnixTimestamp(MAX_UNIX_TIMET);

    
    
    tzOffsetFromUnixTimestamp(opposite);
    tzOffsetFromUnixTimestamp(undesiredTimestamp);
    tzOffsetFromUnixTimestamp(opposite);
    tzOffsetFromUnixTimestamp(undesiredTimestamp);
  }

  function computeCanonicalTZOffset(timestamp)
  {
    clearDSTOffsetCache(timestamp);
    return tzOffsetFromUnixTimestamp(timestamp);
  }

  var TEST_TIMESTAMPS_SECONDS =
    [
     
     0,
     RANGE_EXPANSION_AMOUNT,
     MAX_UNIX_TIMET,
    ];

  var ONE_DAY = 24 * 60 * 60;
  var EIGHTY_THREE_HOURS = 83 * 60 * 60;
  var NINETY_EIGHT_HOURS = 98 * 60 * 60;
  function nextIncrement(i)
  {
    return i === EIGHTY_THREE_HOURS ? NINETY_EIGHT_HOURS : EIGHTY_THREE_HOURS;
  }

  
  
  
  
  
  var DST_CHANGE_DATE = 1268553600; 
  for (var t = DST_CHANGE_DATE - 67 * ONE_DAY,
           i = nextIncrement(NINETY_EIGHT_HOURS),
           end = DST_CHANGE_DATE + 67 * ONE_DAY;
       t < end;
       i = nextIncrement(i), t += i)
  {
    TEST_TIMESTAMPS_SECONDS.push(t);
  }

  var TEST_TIMESTAMPS =
    TEST_TIMESTAMPS_SECONDS.map(function(v) { return v * 1000; });

  



  
  var CORRECT_TZOFFSETS = TEST_TIMESTAMPS.map(computeCanonicalTZOffset);

  
  
  
  

  var sz = TEST_TIMESTAMPS.length;
  var start = Math.floor((part - 1) / parts * sz);
  var end = Math.floor(part / parts * sz);

  print("Exhaustively testing timestamps " +
        "[" + start + ", " + end + ") of " + sz + "...");

  try
  {
    for (var i = start; i < end; i++)
    {
      print("Testing timestamp " + i + "...");

      var t1 = TEST_TIMESTAMPS[i];
      for (var j = 0; j < sz; j++)
      {
        var t2 = TEST_TIMESTAMPS[j];
        for (var k = 0; k < sz; k++)
        {
          var t3 = TEST_TIMESTAMPS[k];
          for (var w = 0; w < sz; w++)
          {
            var t4 = TEST_TIMESTAMPS[w];

            clearDSTOffsetCache(t1);

            var tzo1 = tzOffsetFromUnixTimestamp(t1);
            var tzo2 = tzOffsetFromUnixTimestamp(t2);
            var tzo3 = tzOffsetFromUnixTimestamp(t3);
            var tzo4 = tzOffsetFromUnixTimestamp(t4);

            assertEq(tzo1, CORRECT_TZOFFSETS[i]);
            assertEq(tzo2, CORRECT_TZOFFSETS[j]);
            assertEq(tzo3, CORRECT_TZOFFSETS[k]);
            assertEq(tzo4, CORRECT_TZOFFSETS[w]);
          }
        }
      }
    }
  }
  catch (e)
  {
    assertEq(true, false,
             "Error when testing with timestamps " +
             i + ", " + j + ", " + k + ", " + w +
             " (" + t1 + ", " + t2 + ", " + t3 + ", " + t4 + ")!");
  }

  reportCompare(true, true);
  print("All tests passed!");
}
