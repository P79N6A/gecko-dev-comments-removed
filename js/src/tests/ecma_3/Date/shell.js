





































gTestsubsuite = 'Date';





var msPerDay =   86400000;
var HoursPerDay =  24;
var MinutesPerHour = 60;
var SecondsPerMinute = 60;
var msPerSecond =  1000;
var msPerMinute =  60000;  
var msPerHour =   3600000; 
var TZ_DIFF = getTimeZoneDiff();  
var TZ_ADJUST = TZ_DIFF * msPerHour;
var TZ_PST = -8;  
var PST_DIFF = TZ_DIFF - TZ_PST;  
var TIME_1970  = 0;
var TIME_2000  = 946684800000;
var TIME_1900  = -2208988800000;
var UTC_29_FEB_2000 = TIME_2000 + 31*msPerDay + 28*msPerDay;
var UTC_1_JAN_2005 = TIME_2000 + TimeInYear(2000) + TimeInYear(2001) +
  TimeInYear(2002) + TimeInYear(2003) + TimeInYear(2004);
var now = new Date();
var TIME_NOW = now.valueOf();  
                               







function getTimeZoneDiff()
{
  return -((new Date(2000, 1, 1)).getTimezoneOffset())/60;
}






function adjustResultArray(ResultArray, msMode)
{
  
  if (!PST_DIFF) {return;}

  












  if (msMode)
  {
    
    
    var t = ResultArray[TIME]  +  TZ_DIFF*msPerHour;

    
    ResultArray[HOURS] = HourFromTime(t);
    ResultArray[DAY] = WeekDay(t);
    ResultArray[DATE] = DateFromTime(t);
    ResultArray[MONTH] = MonthFromTime(t);
    ResultArray[YEAR] = YearFromTime(t); 
  }
  else
  {
    
    
    var t = ResultArray[TIME]  -  PST_DIFF*msPerHour;

    
    ResultArray[TIME] = t;
    ResultArray[UTC_HOURS] = HourFromTime(t);
    ResultArray[UTC_DAY] = WeekDay(t);
    ResultArray[UTC_DATE] = DateFromTime(t);
    ResultArray[UTC_MONTH] = MonthFromTime(t);
    ResultArray[UTC_YEAR] = YearFromTime(t);
  }
}


function Day( t ) {
  return ( Math.floor(t/msPerDay ) );
}
function DaysInYear( y ) {
  if ( y % 4 != 0 ) {
    return 365;
  }
  if ( (y % 4 == 0) && (y % 100 != 0) ) {
    return 366;
  }
  if ( (y % 100 == 0) && (y % 400 != 0) ) {
    return 365;
  }
  if ( (y % 400 == 0) ){
    return 366;
  } else {
    return "ERROR: DaysInYear(" + y + ") case not covered";
  }
}
function TimeInYear( y ) {
  return ( DaysInYear(y) * msPerDay );
}
function DayNumber( t ) {
  return ( Math.floor( t / msPerDay ) );
}
function TimeWithinDay( t ) {

  var r = t % msPerDay;

  if (r < 0)
  {
    r += msPerDay;
  }
  return r;

}
function YearNumber( t ) {
}
function TimeFromYear( y ) {
  return ( msPerDay * DayFromYear(y) );
}
function DayFromYear( y ) {
  return ( 365*(y-1970) +
           Math.floor((y-1969)/4) -
           Math.floor((y-1901)/100) +
           Math.floor((y-1601)/400) );
}
function InLeapYear( t ) {
  if ( DaysInYear(YearFromTime(t)) == 365 ) {
    return 0;
  }
  if ( DaysInYear(YearFromTime(t)) == 366 ) {
    return 1;
  } else {
    return "ERROR:  InLeapYear("+ t + ") case not covered";
  }
}
function YearFromTime( t ) {
  t = Number( t );
  var sign = ( t < 0 ) ? -1 : 1;
  var year = ( sign < 0 ) ? 1969 : 1970;
  for ( var timeToTimeZero = t; ;  ) {
    
    timeToTimeZero -= sign * TimeInYear(year)

      
      if ( sign < 0 ) {
        if ( sign * timeToTimeZero <= 0 ) {
          break;
        } else {
          year += sign;
        }
      } else {
        if ( sign * timeToTimeZero < 0 ) {
          break;
        } else {
          year += sign;
        }
      }
  }
  return ( year );
}
function MonthFromTime( t ) {
  
  var day = DayWithinYear( t );
  var leap = InLeapYear(t);

  if ( (0 <= day) && (day < 31) ) {
    return 0;
  }
  if ( (31 <= day) && (day < (59+leap)) ) {
    return 1;
  }
  if ( ((59+leap) <= day) && (day < (90+leap)) ) {
    return 2;
  }
  if ( ((90+leap) <= day) && (day < (120+leap)) ) {
    return 3;
  }
  if ( ((120+leap) <= day) && (day < (151+leap)) ) {
    return 4;
  }
  if ( ((151+leap) <= day) && (day < (181+leap)) ) {
    return 5;
  }
  if ( ((181+leap) <= day) && (day < (212+leap)) ) {
    return 6;
  }
  if ( ((212+leap) <= day) && (day < (243+leap)) ) {
    return 7;
  }
  if ( ((243+leap) <= day) && (day < (273+leap)) ) {
    return 8;
  }
  if ( ((273+leap) <= day) && (day < (304+leap)) ) {
    return 9;
  }
  if ( ((304+leap) <= day) && (day < (334+leap)) ) {
    return 10;
  }
  if ( ((334+leap) <= day) && (day < (365+leap)) ) {
    return 11;
  } else {
    return "ERROR: MonthFromTime("+t+") not known";
  }
}
function DayWithinYear( t ) {
  return( Day(t) - DayFromYear(YearFromTime(t)));
}
function DateFromTime( t ) {
  var day = DayWithinYear(t);
  var month = MonthFromTime(t);

  if ( month == 0 ) {
    return ( day + 1 );
  }
  if ( month == 1 ) {
    return ( day - 30 );
  }
  if ( month == 2 ) {
    return ( day - 58 - InLeapYear(t) );
  }
  if ( month == 3 ) {
    return ( day - 89 - InLeapYear(t));
  }
  if ( month == 4 ) {
    return ( day - 119 - InLeapYear(t));
  }
  if ( month == 5 ) {
    return ( day - 150- InLeapYear(t));
  }
  if ( month == 6 ) {
    return ( day - 180- InLeapYear(t));
  }
  if ( month == 7 ) {
    return ( day - 211- InLeapYear(t));
  }
  if ( month == 8 ) {
    return ( day - 242- InLeapYear(t));
  }
  if ( month == 9 ) {
    return ( day - 272- InLeapYear(t));
  }
  if ( month == 10 ) {
    return ( day - 303- InLeapYear(t));
  }
  if ( month == 11 ) {
    return ( day - 333- InLeapYear(t));
  }

  return ("ERROR:  DateFromTime("+t+") not known" );
}
function WeekDay( t ) {
  var weekday = (Day(t)+4) % 7;
  return( weekday < 0 ? 7 + weekday : weekday );
}



function HourFromTime( t ) {
  var h = Math.floor( t / msPerHour ) % HoursPerDay;
  return ( (h<0) ? HoursPerDay + h : h  );
}
function MinFromTime( t ) {
  var min = Math.floor( t / msPerMinute ) % MinutesPerHour;
  return( ( min < 0 ) ? MinutesPerHour + min : min  );
}
function SecFromTime( t ) {
  var sec = Math.floor( t / msPerSecond ) % SecondsPerMinute;
  return ( (sec < 0 ) ? SecondsPerMinute + sec : sec );
}
function msFromTime( t ) {
  var ms = t % msPerSecond;
  return ( (ms < 0 ) ? msPerSecond + ms : ms );
}
function LocalTZA() {
  return ( TZ_DIFF * msPerHour );
}
function UTC( t ) {
  return ( t - LocalTZA() - DaylightSavingTA(t - LocalTZA()) );
}

function DaylightSavingTA( t ) {
  t = t - LocalTZA();

  var dst_start = GetDSTStart(t);
  var dst_end   = GetDSTEnd(t);

  if ( t >= dst_start && t < dst_end )
    return msPerHour;

  return 0;
}

function GetFirstSundayInMonth( t, m ) {
  var year = YearFromTime(t);
  var leap = InLeapYear(t);





  
  var time = TimeFromYear(year);
  for (var i = 0; i < m; ++i)
  {
    time += TimeInMonth(i, leap);
  }

  for ( var first_sunday = time; WeekDay(first_sunday) > 0;
        first_sunday += msPerDay )
  {
    ;
  }

  return first_sunday;
}

function GetLastSundayInMonth( t, m ) {
  var year = YearFromTime(t);
  var leap = InLeapYear(t);





  
  var time = TimeFromYear(year);
  for (var i = 0; i <= m; ++i)
  {
    time += TimeInMonth(i, leap);
  }
  
  time -= msPerDay;

  for ( var last_sunday = time; WeekDay(last_sunday) > 0;
        last_sunday -= msPerDay )
  {
    ;
  }
  return last_sunday;
}

































function GetDSTStart( t )
{
  return (GetFirstSundayInMonth(t, 2) + 7*msPerDay + 2*msPerHour - LocalTZA());
}

function GetDSTEnd( t )
{
  return (GetFirstSundayInMonth(t, 10) + 2*msPerHour - LocalTZA());
}

function GetOldDSTStart( t )
{
  return (GetFirstSundayInMonth(t, 3) + 2*msPerHour - LocalTZA());
}

function GetOldDSTEnd( t )
{
  return (GetLastSundayInMonth(t, 9) + 2*msPerHour - LocalTZA());
}

function LocalTime( t ) {
  return ( t + LocalTZA() + DaylightSavingTA(t) );
}
function MakeTime( hour, min, sec, ms ) {
  if ( isNaN( hour ) || isNaN( min ) || isNaN( sec ) || isNaN( ms ) ) {
    return Number.NaN;
  }

  hour = ToInteger(hour);
  min  = ToInteger( min);
  sec  = ToInteger( sec);
  ms  = ToInteger( ms );

  return( (hour*msPerHour) + (min*msPerMinute) +
          (sec*msPerSecond) + ms );
}
function MakeDay( year, month, date ) {
  if ( isNaN(year) || isNaN(month) || isNaN(date) ) {
    return Number.NaN;
  }
  year = ToInteger(year);
  month = ToInteger(month);
  date = ToInteger(date );

  var sign = ( year < 1970 ) ? -1 : 1;
  var t =    ( year < 1970 ) ? 1 :  0;
  var y =    ( year < 1970 ) ? 1969 : 1970;

  var result5 = year + Math.floor( month/12 );
  var result6 = month % 12;

  if ( year < 1970 ) {
    for ( y = 1969; y >= year; y += sign ) {
      t += sign * TimeInYear(y);
    }
  } else {
    for ( y = 1970 ; y < year; y += sign ) {
      t += sign * TimeInYear(y);
    }
  }

  var leap = InLeapYear( t );

  for ( var m = 0; m < month; m++ ) {
    t += TimeInMonth( m, leap );
  }

  if ( YearFromTime(t) != result5 ) {
    return Number.NaN;
  }
  if ( MonthFromTime(t) != result6 ) {
    return Number.NaN;
  }
  if ( DateFromTime(t) != 1 ) {
    return Number.NaN;
  }

  return ( (Day(t)) + date - 1 );
}
function TimeInMonth( month, leap ) {
  
  
  

  if ( month == 3 || month == 5 || month == 8 || month == 10 ) {
    return ( 30*msPerDay );
  }

  
  if ( month == 0 || month == 2 || month == 4 || month == 6 ||
       month == 7 || month == 9 || month == 11 ) {
    return ( 31*msPerDay );
  }

  
  return ( (leap == 0) ? 28*msPerDay : 29*msPerDay );
}
function MakeDate( day, time ) {
  if ( day == Number.POSITIVE_INFINITY ||
       day == Number.NEGATIVE_INFINITY ) {
    return Number.NaN;
  }
  if ( time == Number.POSITIVE_INFINITY ||
       time == Number.NEGATIVE_INFINITY ) {
    return Number.NaN;
  }
  return ( day * msPerDay ) + time;
}
function TimeClip( t ) {
  if ( isNaN( t ) ) {
    return ( Number.NaN );
  }
  if ( Math.abs( t ) > 8.64e15 ) {
    return ( Number.NaN );
  }

  return ( ToInteger( t ) );
}
function ToInteger( t ) {
  t = Number( t );

  if ( isNaN( t ) ){
    return ( Number.NaN );
  }
  if ( t == 0 || t == -0 ||
       t == Number.POSITIVE_INFINITY || t == Number.NEGATIVE_INFINITY ) {
    return 0;
  }

  var sign = ( t < 0 ) ? -1 : 1;

  return ( sign * Math.floor( Math.abs( t ) ) );
}
function Enumerate ( o ) {
  var p;
  for ( p in o ) {
    print( p +": " + o[p] );
  }
}

