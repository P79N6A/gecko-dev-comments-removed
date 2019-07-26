






function compareArray(aExpected, aActual) {
    if (aActual.length != aExpected.length) {
        return false;
    }

    aExpected.sort();
    aActual.sort();

    var s;
    for (var i = 0; i < aExpected.length; i++) {
        if (aActual[i] !== aExpected[i]) {
            return false;
        }
    }
    return true;
}


function arrayContains(arr, expected) {
    var found;
    for (var i = 0; i < expected.length; i++) {
        found = false;
        for (var j = 0; j < arr.length; j++) {
            if (expected[i] === arr[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}


var supportsArrayIndexGettersOnArrays = undefined;
function fnSupportsArrayIndexGettersOnArrays() {
    if (typeof supportsArrayIndexGettersOnArrays !== "undefined") {
        return supportsArrayIndexGettersOnArrays;
    }

    supportsArrayIndexGettersOnArrays = false;

    if (fnExists(Object.defineProperty)) {
        var arr = [];
        Object.defineProperty(arr, "0", {
            get: function() {
                supportsArrayIndexGettersOnArrays = true;
                return 0;
            }
        });
        var res = arr[0];
    }

    return supportsArrayIndexGettersOnArrays;
}


var supportsArrayIndexGettersOnObjects = undefined;
function fnSupportsArrayIndexGettersOnObjects() {
    if (typeof supportsArrayIndexGettersOnObjects !== "undefined")
        return supportsArrayIndexGettersOnObjects;

    supportsArrayIndexGettersOnObjects = false;

    if (fnExists(Object.defineProperty)) {
        var obj = {};
        Object.defineProperty(obj, "0", {
            get: function() {
                supportsArrayIndexGettersOnObjects = true;
                return 0;
            }
        });
        var res = obj[0];
    }

    return supportsArrayIndexGettersOnObjects;
}


function ConvertToFileUrl(pathStr) {
    return "file:" + pathStr.replace(/\\/g, "/");
}


function fnExists() {
    for (var i = 0; i < arguments.length; i++) {
        if (typeof (arguments[i]) !== "function") return false;
    }
    return true;
}


var __globalObject = Function("return this;")();
function fnGlobalObject() {
     return __globalObject;
}


function fnSupportsStrict() {
    "use strict";
    try {
        eval('with ({}) {}');
        return false;
    } catch (e) {
        return true;
    }
}





function dataPropertyAttributesAreCorrect(obj,
                                          name,
                                          value,
                                          writable,
                                          enumerable,
                                          configurable) {
    var attributesCorrect = true;

    if (obj[name] !== value) {
        if (typeof obj[name] === "number" &&
            isNaN(obj[name]) &&
            typeof value === "number" &&
            isNaN(value)) {
            
        } else {
            attributesCorrect = false;
        }
    }

    try {
        if (obj[name] === "oldValue") {
            obj[name] = "newValue";
        } else {
            obj[name] = "OldValue";
        }
    } catch (we) {
    }

    var overwrited = false;
    if (obj[name] !== value) {
        if (typeof obj[name] === "number" &&
            isNaN(obj[name]) &&
            typeof value === "number" &&
            isNaN(value)) {
            
        } else {
            overwrited = true;
        }
    }
    if (overwrited !== writable) {
        attributesCorrect = false;
    }

    var enumerated = false;
    for (var prop in obj) {
        if (obj.hasOwnProperty(prop) && prop === name) {
            enumerated = true;
        }
    }

    if (enumerated !== enumerable) {
        attributesCorrect = false;
    }


    var deleted = false;

    try {
        delete obj[name];
    } catch (de) {
    }
    if (!obj.hasOwnProperty(name)) {
        deleted = true;
    }
    if (deleted !== configurable) {
        attributesCorrect = false;
    }

    return attributesCorrect;
}





function accessorPropertyAttributesAreCorrect(obj,
                                              name,
                                              get,
                                              set,
                                              setVerifyHelpProp,
                                              enumerable,
                                              configurable) {
    var attributesCorrect = true;

    if (get !== undefined) {
        if (obj[name] !== get()) {
            if (typeof obj[name] === "number" &&
                isNaN(obj[name]) &&
                typeof get() === "number" &&
                isNaN(get())) {
                
            } else {
                attributesCorrect = false;
            }
        }
    } else {
        if (obj[name] !== undefined) {
            attributesCorrect = false;
        }
    }

    try {
        var desc = Object.getOwnPropertyDescriptor(obj, name);
        if (typeof desc.set === "undefined") {
            if (typeof set !== "undefined") {
                attributesCorrect = false;
            }
        } else {
            obj[name] = "toBeSetValue";
            if (obj[setVerifyHelpProp] !== "toBeSetValue") {
                attributesCorrect = false;
            }
        }
    } catch (se) {
        throw se;
    }


    var enumerated = false;
    for (var prop in obj) {
        if (obj.hasOwnProperty(prop) && prop === name) {
            enumerated = true;
        }
    }

    if (enumerated !== enumerable) {
        attributesCorrect = false;
    }


    var deleted = false;
    try {
        delete obj[name];
    } catch (de) {
        throw de;
    }
    if (!obj.hasOwnProperty(name)) {
        deleted = true;
    }
    if (deleted !== configurable) {
        attributesCorrect = false;
    }

    return attributesCorrect;
}


var NotEarlyErrorString = "NotEarlyError";
var EarlyErrorRePat = "^((?!" + NotEarlyErrorString + ").)*$";
var NotEarlyError = new Error(NotEarlyErrorString);





function Test262Error(message) {
    if (message) this.message = message;
}

Test262Error.prototype.toString = function () {
    return "Test262 Error: " + this.message;
};

function testFailed(message) {
    throw new Test262Error(message);
}


function testPrint(message) {

}



function $PRINT(message) {

}

function $INCLUDE(message) { }
function $ERROR(message) {
    testFailed(message);
}

function $FAIL(message) {
    testFailed(message);
}












function getPrecision(num) {
    
    

    var log2num = Math.log(Math.abs(num)) / Math.LN2;
    var pernum = Math.ceil(log2num);
    return (2 * Math.pow(2, -52 + pernum));
    
}






var prec;
function isEqual(num1, num2) {
    if ((num1 === Infinity) && (num2 === Infinity)) {
        return (true);
    }
    if ((num1 === -Infinity) && (num2 === -Infinity)) {
        return (true);
    }
    prec = getPrecision(Math.min(Math.abs(num1), Math.abs(num2)));
    return (Math.abs(num1 - num2) <= prec);
    
}





function ToInteger(p) {
    var x = Number(p);

    if (isNaN(x)) {
        return +0;
    }

    if ((x === +0)
  || (x === -0)
  || (x === Number.POSITIVE_INFINITY)
  || (x === Number.NEGATIVE_INFINITY)) {
        return x;
    }

    var sign = (x < 0) ? -1 : 1;

    return (sign * Math.floor(Math.abs(x)));
}





var HoursPerDay = 24;
var MinutesPerHour = 60;
var SecondsPerMinute = 60;

var msPerDay = 86400000;
var msPerSecond = 1000;
var msPerMinute = 60000;
var msPerHour = 3600000;

var date_1899_end = -2208988800001;
var date_1900_start = -2208988800000;
var date_1969_end = -1;
var date_1970_start = 0;
var date_1999_end = 946684799999;
var date_2000_start = 946684800000;
var date_2099_end = 4102444799999;
var date_2100_start = 4102444800000;





var $LocalTZ,
    $DST_start_month,
    $DST_start_sunday,
    $DST_start_hour,
    $DST_start_minutes,
    $DST_end_month,
    $DST_end_sunday,
    $DST_end_hour,
    $DST_end_minutes;

(function () {
    



    var findNearestDateBefore = function(start, predicate) {
        var current = start;
        var month = 1000 * 60 * 60 * 24 * 30;
        for (var step = month; step > 0; step = Math.floor(step / 3)) {
            if (!predicate(current)) {
                while (!predicate(current))
                    current = new Date(current.getTime() + step);
                    current = new Date(current.getTime() - step);
                }
        }
        while (!predicate(current)) {
            current = new Date(current.getTime() + 1);
        }
        return current;
    };

    var juneDate = new Date(2000, 5, 20, 0, 0, 0, 0);
    var decemberDate = new Date(2000, 11, 20, 0, 0, 0, 0);
    var juneOffset = juneDate.getTimezoneOffset();
    var decemberOffset = decemberDate.getTimezoneOffset();
    var isSouthernHemisphere = (juneOffset > decemberOffset);
    var winterTime = isSouthernHemisphere ? juneDate : decemberDate;
    var summerTime = isSouthernHemisphere ? decemberDate : juneDate;

    var dstStart = findNearestDateBefore(winterTime, function (date) {
        return date.getTimezoneOffset() == summerTime.getTimezoneOffset();
    });
    $DST_start_month = dstStart.getMonth();
    $DST_start_sunday = dstStart.getDate() > 15 ? '"last"' : '"first"';
    $DST_start_hour = dstStart.getHours();
    $DST_start_minutes = dstStart.getMinutes();

    var dstEnd = findNearestDateBefore(summerTime, function (date) {
        return date.getTimezoneOffset() == winterTime.getTimezoneOffset();
    });
    $DST_end_month = dstEnd.getMonth();
    $DST_end_sunday = dstEnd.getDate() > 15 ? '"last"' : '"first"';
    $DST_end_hour = dstEnd.getHours();
    $DST_end_minutes = dstEnd.getMinutes();

    return;
})();







function Day(t) {
  return Math.floor(t/msPerDay);
}

function TimeWithinDay(t) {
  return t%msPerDay;
}


function DaysInYear(y){
  if(y%4 != 0) return 365;
  if(y%4 == 0 && y%100 != 0) return 366;
  if(y%100 == 0 && y%400 != 0) return 365;
  if(y%400 == 0) return 366;
}

function DayFromYear(y) {
  return (365*(y-1970)
          + Math.floor((y-1969)/4)
          - Math.floor((y-1901)/100)
          + Math.floor((y-1601)/400));
}

function TimeFromYear(y){
  return msPerDay*DayFromYear(y);
}

function YearFromTime(t) {
  t = Number(t);
  var sign = ( t < 0 ) ? -1 : 1;
  var year = ( sign < 0 ) ? 1969 : 1970;

  for(var time = 0;;year += sign){
    time = TimeFromYear(year);

    if(sign > 0 && time > t){
      year -= sign;
      break;
    }
    else if(sign < 0 && time <= t){
      break;
    }
  };
  return year;
}

function InLeapYear(t){
  if(DaysInYear(YearFromTime(t)) == 365)
    return 0;

  if(DaysInYear(YearFromTime(t)) == 366)
    return 1;
}

function DayWithinYear(t) {
  return Day(t)-DayFromYear(YearFromTime(t));
}


function MonthFromTime(t){
  var day = DayWithinYear(t);
  var leap = InLeapYear(t);

  if((0 <= day) && (day < 31)) return 0;
  if((31 <= day) && (day < (59+leap))) return 1;
  if(((59+leap) <= day) && (day < (90+leap))) return 2;
  if(((90+leap) <= day) && (day < (120+leap))) return 3;
  if(((120+leap) <= day) && (day < (151+leap))) return 4;
  if(((151+leap) <= day) && (day < (181+leap))) return 5;
  if(((181+leap) <= day) && (day < (212+leap))) return 6;
  if(((212+leap) <= day) && (day < (243+leap))) return 7;
  if(((243+leap) <= day) && (day < (273+leap))) return 8;
  if(((273+leap) <= day) && (day < (304+leap))) return 9;
  if(((304+leap) <= day) && (day < (334+leap))) return 10;
  if(((334+leap) <= day) && (day < (365+leap))) return 11;
}


function DateFromTime(t) {
  var day = DayWithinYear(t);
  var month = MonthFromTime(t);
  var leap = InLeapYear(t);

  if(month == 0) return day+1;
  if(month == 1) return day-30;
  if(month == 2) return day-58-leap;
  if(month == 3) return day-89-leap;
  if(month == 4) return day-119-leap;
  if(month == 5) return day-150-leap;
  if(month == 6) return day-180-leap;
  if(month == 7) return day-211-leap;
  if(month == 8) return day-242-leap;
  if(month == 9) return day-272-leap;
  if(month == 10) return day-303-leap;
  if(month == 11) return day-333-leap;
}


function WeekDay(t) {
  var weekday = (Day(t)+4)%7;
  return (weekday < 0 ? 7+weekday : weekday);
}


$LocalTZ = (new Date()).getTimezoneOffset() / -60;
if (DaylightSavingTA((new Date()).valueOf()) !== 0) {
   $LocalTZ -= 1;
}
var LocalTZA = $LocalTZ*msPerHour;

function DaysInMonth(m, leap) {
  m = m%12;

  
  if(m == 3 || m == 5 || m == 8 || m == 10 ) {
    return 30;
  }

  
  if(m == 0 || m == 2 || m == 4 || m == 6 || m == 7 || m == 9 || m == 11){
    return 31;
  }

  
  return 28+leap;
}

function GetSundayInMonth(t, m, count){
    var year = YearFromTime(t);
    var tempDate;

    if (count==='"first"') {
        for (var d=1; d <= DaysInMonth(m, InLeapYear(t)); d++) {
            tempDate = new Date(year, m, d);
            if (tempDate.getDay()===0) {
                return tempDate.valueOf();
            }
        }
    } else if(count==='"last"') {
        for (var d=DaysInMonth(m, InLeapYear(t)); d>0; d--) {
            tempDate = new Date(year, m, d);
            if (tempDate.getDay()===0) {
                return tempDate.valueOf();
            }
        }
    }
    throw new Error("Unsupported 'count' arg:" + count);
}







































function DaylightSavingTA(t) {


  var DST_start = GetSundayInMonth(t, $DST_start_month, $DST_start_sunday) +
                  $DST_start_hour*msPerHour +
                  $DST_start_minutes*msPerMinute;

  var k = new Date(DST_start);

  var DST_end   = GetSundayInMonth(t, $DST_end_month, $DST_end_sunday) +
                  $DST_end_hour*msPerHour +
                  $DST_end_minutes*msPerMinute;

  if ( t >= DST_start && t < DST_end ) {
    return msPerHour;
  } else {
    return 0;
  }
}


function LocalTime(t){
  return t+LocalTZA+DaylightSavingTA(t);
}

function UTC(t) {
  return t-LocalTZA-DaylightSavingTA(t-LocalTZA);
}


function HourFromTime(t){
  return Math.floor(t/msPerHour)%HoursPerDay;
}

function MinFromTime(t){
  return Math.floor(t/msPerMinute)%MinutesPerHour;
}

function SecFromTime(t){
  return Math.floor(t/msPerSecond)%SecondsPerMinute;
}

function msFromTime(t){
  return t%msPerSecond;
}


function MakeTime(hour, min, sec, ms){
  if ( !isFinite(hour) || !isFinite(min) || !isFinite(sec) || !isFinite(ms)) {
    return Number.NaN;
  }

  hour = ToInteger(hour);
  min  = ToInteger(min);
  sec  = ToInteger(sec);
  ms   = ToInteger(ms);

  return ((hour*msPerHour) + (min*msPerMinute) + (sec*msPerSecond) + ms);
}


function MakeDay(year, month, date) {
  if ( !isFinite(year) || !isFinite(month) || !isFinite(date)) {
    return Number.NaN;
  }

  year = ToInteger(year);
  month = ToInteger(month);
  date = ToInteger(date );

  var result5 = year + Math.floor(month/12);
  var result6 = month%12;

  var sign = ( year < 1970 ) ? -1 : 1;
  var t =    ( year < 1970 ) ? 1 :  0;
  var y =    ( year < 1970 ) ? 1969 : 1970;

  if( sign == -1 ){
    for ( y = 1969; y >= year; y += sign ) {
      t += sign * DaysInYear(y)*msPerDay;
    }
  } else {
    for ( y = 1970 ; y < year; y += sign ) {
      t += sign * DaysInYear(y)*msPerDay;
    }
  }

  var leap = 0;
  for ( var m = 0; m < month; m++ ) {
    
    leap = InLeapYear(t);
    t += DaysInMonth(m, leap)*msPerDay;
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

  return Day(t)+date-1;
}


function MakeDate( day, time ) {
  if(!isFinite(day) || !isFinite(time)) {
    return Number.NaN;
  }

  return day*msPerDay+time;
}


function TimeClip(time) {
  if(!isFinite(time) || Math.abs(time) > 8.64e15){
    return Number.NaN;
  }

  return ToInteger(time);
}





function ConstructDate(year, month, date, hours, minutes, seconds, ms){
  














  var r1 = Number(year);
  var r2 = Number(month);
  var r3 = ((date && arguments.length > 2) ? Number(date) : 1);
  var r4 = ((hours && arguments.length > 3) ? Number(hours) : 0);
  var r5 = ((minutes && arguments.length > 4) ? Number(minutes) : 0);
  var r6 = ((seconds && arguments.length > 5) ? Number(seconds) : 0);
  var r7 = ((ms && arguments.length > 6) ? Number(ms) : 0);

  var r8 = r1;

  if(!isNaN(r1) && (0 <= ToInteger(r1)) && (ToInteger(r1) <= 99))
    r8 = 1900+r1;

  var r9 = MakeDay(r8, r2, r3);
  var r10 = MakeTime(r4, r5, r6, r7);
  var r11 = MakeDate(r9, r10);

  var retVal = TimeClip(UTC(r11));
  return retVal;
}




















































































function runTestCase(testcase) {
    if (testcase() !== true) {
        $ERROR("Test case returned non-true value!");
    }
}











testPassesUnlessItThrows();





function $ERROR(msg)
{
  throw new Error("Test262 error: " + msg);
}





function $INCLUDE(file)
{
  load("supporting/" + file);
}




var fnGlobalObject = (function()
{
  var global = Function("return this")();
  return function fnGlobalObject() { return global; };
})();
