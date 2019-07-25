





























const NONE        = 0;
const READ_ONLY   = 1;
const DONT_ENUM   = 2;
const DONT_DELETE = 4;


const GETTER = 0;
const SETTER = 1;


const kApiTagOffset                 = 0;
const kApiPropertyListOffset        = 1;
const kApiSerialNumberOffset        = 2;
const kApiConstructorOffset         = 2;
const kApiPrototypeTemplateOffset   = 5;
const kApiParentTemplateOffset      = 6;
const kApiFlagOffset                = 14;

const NO_HINT     = 0;
const NUMBER_HINT = 1;
const STRING_HINT = 2;

const kFunctionTag  = 0;
const kNewObjectTag = 1;


const HoursPerDay      = 24;
const MinutesPerHour   = 60;
const SecondsPerMinute = 60;
const msPerSecond      = 1000;
const msPerMinute      = 60000;
const msPerHour        = 3600000;
const msPerDay         = 86400000;
const msPerMonth       = 2592000000;


const kUninitialized = -1;
const kReadOnlyPrototypeBit = 3;  


const kInvalidDate        = 'Invalid Date';
const kDayZeroInJulianDay = 2440588;
const kMonthMask          = 0x1e0;
const kDayMask            = 0x01f;
const kYearShift          = 9;
const kMonthShift         = 5;




const kMinYear  = -1000000;
const kMaxYear  = 1000000;
const kMinMonth = -10000000;
const kMaxMonth = 10000000;


const STRING_TO_REGEXP_CACHE_ID = 0;






macro IS_NULL(arg)              = (arg === null);
macro IS_NULL_OR_UNDEFINED(arg) = (arg == null);
macro IS_UNDEFINED(arg)         = (typeof(arg) === 'undefined');
macro IS_NUMBER(arg)            = (typeof(arg) === 'number');
macro IS_STRING(arg)            = (typeof(arg) === 'string');
macro IS_BOOLEAN(arg)           = (typeof(arg) === 'boolean');
macro IS_OBJECT(arg)            = (%_IsObject(arg));
macro IS_ARRAY(arg)             = (%_IsArray(arg));
macro IS_FUNCTION(arg)          = (%_IsFunction(arg));
macro IS_REGEXP(arg)            = (%_IsRegExp(arg));
macro IS_SET(arg)               = (%_ClassOf(arg) === 'Set');
macro IS_MAP(arg)               = (%_ClassOf(arg) === 'Map');
macro IS_WEAKMAP(arg)           = (%_ClassOf(arg) === 'WeakMap');
macro IS_DATE(arg)              = (%_ClassOf(arg) === 'Date');
macro IS_NUMBER_WRAPPER(arg)    = (%_ClassOf(arg) === 'Number');
macro IS_STRING_WRAPPER(arg)    = (%_ClassOf(arg) === 'String');
macro IS_BOOLEAN_WRAPPER(arg)   = (%_ClassOf(arg) === 'Boolean');
macro IS_ERROR(arg)             = (%_ClassOf(arg) === 'Error');
macro IS_SCRIPT(arg)            = (%_ClassOf(arg) === 'Script');
macro IS_ARGUMENTS(arg)         = (%_ClassOf(arg) === 'Arguments');
macro IS_GLOBAL(arg)            = (%_ClassOf(arg) === 'global');
macro IS_UNDETECTABLE(arg)      = (%_IsUndetectableObject(arg));
macro FLOOR(arg)                = $floor(arg);






macro IS_SPEC_OBJECT(arg)   = (%_IsSpecObject(arg));






macro IS_SPEC_FUNCTION(arg) = (%_ClassOf(arg) === 'Function');


const kBoundFunctionIndex = 0;
const kBoundThisIndex = 1;
const kBoundArgumentsStartIndex = 2;


macro NUMBER_IS_NAN(arg) = (!%_IsSmi(%IS_VAR(arg)) && !(arg == arg));
macro NUMBER_IS_FINITE(arg) = (%_IsSmi(%IS_VAR(arg)) || ((arg == arg) && (arg != 1/0) && (arg != -1/0)));
macro TO_INTEGER(arg) = (%_IsSmi(%IS_VAR(arg)) ? arg : %NumberToInteger(ToNumber(arg)));
macro TO_INTEGER_MAP_MINUS_ZERO(arg) = (%_IsSmi(%IS_VAR(arg)) ? arg : %NumberToIntegerMapMinusZero(ToNumber(arg)));
macro TO_INT32(arg) = (%_IsSmi(%IS_VAR(arg)) ? arg : (arg >> 0));
macro TO_UINT32(arg) = (arg >>> 0);
macro TO_STRING_INLINE(arg) = (IS_STRING(%IS_VAR(arg)) ? arg : NonStringToString(arg));
macro TO_NUMBER_INLINE(arg) = (IS_NUMBER(%IS_VAR(arg)) ? arg : NonNumberToNumber(arg));
macro TO_OBJECT_INLINE(arg) = (IS_SPEC_OBJECT(%IS_VAR(arg)) ? arg : ToObject(arg));
macro JSON_NUMBER_TO_STRING(arg) = ((%_IsSmi(%IS_VAR(arg)) || arg - arg == 0) ? %_NumberToString(arg) : "null");


python macro CHAR_CODE(str) = ord(str[1]);


const REGEXP_NUMBER_OF_CAPTURES = 0;
const REGEXP_FIRST_CAPTURE = 3;



macro NUMBER_OF_CAPTURES(array) = ((array)[0]);


const MAX_TIME_MS = 8640000000000000;

const MAX_TIME_BEFORE_UTC = 8640002592000000;



macro CHECK_DATE(arg) = if (%_ClassOf(arg) !== 'Date') ThrowDateTypeError();
macro LOCAL_DATE_VALUE(arg) = (%_DateField(arg, 0) + %_DateField(arg, 21));
macro UTC_DATE_VALUE(arg)    = (%_DateField(arg, 0));

macro LOCAL_YEAR(arg)        = (%_DateField(arg, 1));
macro LOCAL_MONTH(arg)       = (%_DateField(arg, 2));
macro LOCAL_DAY(arg)         = (%_DateField(arg, 3));
macro LOCAL_WEEKDAY(arg)     = (%_DateField(arg, 4));
macro LOCAL_HOUR(arg)        = (%_DateField(arg, 5));
macro LOCAL_MIN(arg)         = (%_DateField(arg, 6));
macro LOCAL_SEC(arg)         = (%_DateField(arg, 7));
macro LOCAL_MS(arg)          = (%_DateField(arg, 8));
macro LOCAL_DAYS(arg)        = (%_DateField(arg, 9));
macro LOCAL_TIME_IN_DAY(arg) = (%_DateField(arg, 10));

macro UTC_YEAR(arg)        = (%_DateField(arg, 11));
macro UTC_MONTH(arg)       = (%_DateField(arg, 12));
macro UTC_DAY(arg)         = (%_DateField(arg, 13));
macro UTC_WEEKDAY(arg)     = (%_DateField(arg, 14));
macro UTC_HOUR(arg)        = (%_DateField(arg, 15));
macro UTC_MIN(arg)         = (%_DateField(arg, 16));
macro UTC_SEC(arg)         = (%_DateField(arg, 17));
macro UTC_MS(arg)          = (%_DateField(arg, 18));
macro UTC_DAYS(arg)        = (%_DateField(arg, 19));
macro UTC_TIME_IN_DAY(arg) = (%_DateField(arg, 20));

macro TIMEZONE_OFFSET(arg)   = (%_DateField(arg, 21));

macro SET_UTC_DATE_VALUE(arg, value) = (%DateSetValue(arg, value, 1));
macro SET_LOCAL_DATE_VALUE(arg, value) = (%DateSetValue(arg, value, 0));


const LAST_SUBJECT_INDEX = 1;
macro LAST_SUBJECT(array) = ((array)[1]);
macro LAST_INPUT(array) = ((array)[2]);


macro CAPTURE(index) = (3 + (index));
const CAPTURE0 = 3;
const CAPTURE1 = 4;




macro OVERRIDE_MATCH(override) = ((override)[0]);
macro OVERRIDE_POS(override) = ((override)[(override).length - 2]);
macro OVERRIDE_SUBJECT(override) = ((override)[(override).length - 1]);

macro OVERRIDE_CAPTURE(override, index) = ((override)[(index)]);



const IS_ACCESSOR_INDEX = 0;
const VALUE_INDEX = 1;
const GETTER_INDEX = 2;
const SETTER_INDEX = 3;
const WRITABLE_INDEX = 4;
const ENUMERABLE_INDEX = 5;
const CONFIGURABLE_INDEX = 6;



const TYPE_NATIVE = 0;
const TYPE_EXTENSION = 1;
const TYPE_NORMAL = 2;


const COMPILATION_TYPE_HOST = 0;
const COMPILATION_TYPE_EVAL = 1;
const COMPILATION_TYPE_JSON = 2;


const kNoLineNumberInfo = 0;
