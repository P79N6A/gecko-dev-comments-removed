























#include <limits>
#include <stdint.h>
#include <vector>

#include "pkixder.h"
#include "pkixgtest.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::der;
using namespace mozilla::pkix::test;
using namespace std;

class pkixder_universal_types_tests : public ::testing::Test { };

TEST_F(pkixder_universal_types_tests, BooleanTrue01)
{
  const uint8_t DER_BOOLEAN_TRUE_01[] = {
    0x01,                       
    0x01,                       
    0x01                        
  };
  Input input(DER_BOOLEAN_TRUE_01);
  Reader reader(input);
  bool value = false;
  ASSERT_EQ(Result::ERROR_BAD_DER, Boolean(reader, value));
}

TEST_F(pkixder_universal_types_tests, BooleanTrue42)
{
  const uint8_t DER_BOOLEAN_TRUE_42[] = {
    0x01,                       
    0x01,                       
    0x42                        
  };
  Input input(DER_BOOLEAN_TRUE_42);
  Reader reader(input);
  bool value = false;
  ASSERT_EQ(Result::ERROR_BAD_DER, Boolean(reader, value));
}

static const uint8_t DER_BOOLEAN_TRUE[] = {
  0x01,                       
  0x01,                       
  0xff                        
};

TEST_F(pkixder_universal_types_tests, BooleanTrueFF)
{
  Input input(DER_BOOLEAN_TRUE);
  Reader reader(input);
  bool value = false;
  ASSERT_EQ(Success, Boolean(reader, value));
  ASSERT_TRUE(value);
}

TEST_F(pkixder_universal_types_tests, BooleanFalse)
{
  const uint8_t DER_BOOLEAN_FALSE[] = {
    0x01,                       
    0x01,                       
    0x00                        
  };
  Input input(DER_BOOLEAN_FALSE);
  Reader reader(input);

  bool value = true;
  ASSERT_EQ(Success, Boolean(reader, value));
  ASSERT_FALSE(value);
}

TEST_F(pkixder_universal_types_tests, BooleanInvalidLength)
{
  const uint8_t DER_BOOLEAN_INVALID_LENGTH[] = {
    0x01,                       
    0x02,                       
    0x42, 0x42                  
  };
  Input input(DER_BOOLEAN_INVALID_LENGTH);
  Reader reader(input);

  bool value = true;
  ASSERT_EQ(Result::ERROR_BAD_DER, Boolean(reader, value));
}

TEST_F(pkixder_universal_types_tests, BooleanInvalidZeroLength)
{
  const uint8_t DER_BOOLEAN_INVALID_ZERO_LENGTH[] = {
    0x01,                       
    0x00                        
  };
  Input input(DER_BOOLEAN_INVALID_ZERO_LENGTH);
  Reader reader(input);

  bool value = true;
  ASSERT_EQ(Result::ERROR_BAD_DER, Boolean(reader, value));
}






TEST_F(pkixder_universal_types_tests, OptionalBooleanValidEncodings)
{
  {
    const uint8_t DER_OPTIONAL_BOOLEAN_PRESENT_TRUE[] = {
      0x01,                       
      0x01,                       
      0xff                        
    };
    Input input(DER_OPTIONAL_BOOLEAN_PRESENT_TRUE);
    Reader reader(input);
    bool value = false;
    ASSERT_EQ(Success, OptionalBoolean(reader, value)) <<
      "Should accept the only valid encoding of a present OPTIONAL BOOLEAN";
    ASSERT_TRUE(value);
    ASSERT_TRUE(reader.AtEnd());
  }

  {
    
    const uint8_t DER_INTEGER_05[] = {
      0x02,                       
      0x01,                       
      0x05
    };
    Input input(DER_INTEGER_05);
    Reader reader(input);
    bool value = true;
    ASSERT_EQ(Success, OptionalBoolean(reader, value)) <<
      "Should accept a valid encoding of an omitted OPTIONAL BOOLEAN";
    ASSERT_FALSE(value);
    ASSERT_FALSE(reader.AtEnd());
  }

  {
    Input input;
    ASSERT_EQ(Success, input.Init(reinterpret_cast<const uint8_t*>(""), 0));
    Reader reader(input);
    bool value = true;
    ASSERT_EQ(Success, OptionalBoolean(reader, value)) <<
      "Should accept another valid encoding of an omitted OPTIONAL BOOLEAN";
    ASSERT_FALSE(value);
    ASSERT_TRUE(reader.AtEnd());
  }
}

TEST_F(pkixder_universal_types_tests, OptionalBooleanInvalidEncodings)
{
  const uint8_t DER_OPTIONAL_BOOLEAN_PRESENT_FALSE[] = {
    0x01,                       
    0x01,                       
    0x00                        
  };

  {
    Input input(DER_OPTIONAL_BOOLEAN_PRESENT_FALSE);
    Reader reader(input);
    bool value = true;
    ASSERT_EQ(Success, OptionalBoolean(reader, value)) <<
      "Should accept an invalid, default-value encoding of OPTIONAL BOOLEAN";
    ASSERT_FALSE(value);
    ASSERT_TRUE(reader.AtEnd());
  }

  const uint8_t DER_OPTIONAL_BOOLEAN_PRESENT_42[] = {
    0x01,                       
    0x01,                       
    0x42                        
  };

  {
    Input input(DER_OPTIONAL_BOOLEAN_PRESENT_42);
    Reader reader(input);
    bool value;
    ASSERT_EQ(Result::ERROR_BAD_DER, OptionalBoolean(reader, value)) <<
      "Should reject an invalid-valued encoding of OPTIONAL BOOLEAN";
  }
}

TEST_F(pkixder_universal_types_tests, Enumerated)
{
  const uint8_t DER_ENUMERATED[] = {
    0x0a,                       
    0x01,                       
    0x42                        
  };
  Input input(DER_ENUMERATED);
  Reader reader(input);

  uint8_t value = 0;
  ASSERT_EQ(Success, Enumerated(reader, value));
  ASSERT_EQ(0x42, value);
}

TEST_F(pkixder_universal_types_tests, EnumeratedNotShortestPossibleDER)
{
  const uint8_t DER_ENUMERATED[] = {
    0x0a,                       
    0x02,                       
    0x00, 0x01                  
  };
  Input input(DER_ENUMERATED);
  Reader reader(input);

  uint8_t value = 0;
  ASSERT_EQ(Result::ERROR_BAD_DER, Enumerated(reader, value));
}

TEST_F(pkixder_universal_types_tests, EnumeratedOutOfAcceptedRange)
{
  
  
  
  
  const uint8_t DER_ENUMERATED_INVALID_LENGTH[] = {
    0x0a,                       
    0x02,                       
    0x12, 0x34                  
  };
  Input input(DER_ENUMERATED_INVALID_LENGTH);
  Reader reader(input);

  uint8_t value = 0;
  ASSERT_EQ(Result::ERROR_BAD_DER, Enumerated(reader, value));
}

TEST_F(pkixder_universal_types_tests, EnumeratedInvalidZeroLength)
{
  const uint8_t DER_ENUMERATED_INVALID_ZERO_LENGTH[] = {
    0x0a,                       
    0x00                        
  };
  Input input(DER_ENUMERATED_INVALID_ZERO_LENGTH);
  Reader reader(input);

  uint8_t value = 0;
  ASSERT_EQ(Result::ERROR_BAD_DER, Enumerated(reader, value));
}























#define TWO_CHARS(t) \
  static_cast<uint8_t>('0' + (static_cast<uint8_t>(t) / 10u)), \
  static_cast<uint8_t>('0' + (static_cast<uint8_t>(t) % 10u))


template <uint16_t LENGTH>
Result
TimeChoiceForEquivalentUTCTime(const uint8_t (&generalizedTimeDER)[LENGTH],
                                Time& value)
{
  static_assert(LENGTH >= 4,
                "TimeChoiceForEquivalentUTCTime input too small");
  uint8_t utcTimeDER[LENGTH - 2];
  utcTimeDER[0] = 0x17; 
  utcTimeDER[1] = LENGTH - 1 - 1 - 2;
  
  for (size_t i = 2; i < LENGTH - 2; ++i) {
    utcTimeDER[i] = generalizedTimeDER[i + 2];
  }

  Input input(utcTimeDER);
  Reader reader(input);
  return TimeChoice(reader, value);
}

template <uint16_t LENGTH>
void
ExpectGoodTime(Time expectedValue,
               const uint8_t (&generalizedTimeDER)[LENGTH])
{
  
  {
    Input input(generalizedTimeDER);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Success, GeneralizedTime(reader, value));
    EXPECT_EQ(expectedValue, value);
  }

  
  {
    Input input(generalizedTimeDER);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Success, TimeChoice(reader, value));
    EXPECT_EQ(expectedValue, value);
  }

  
  {
    Time value(Time::uninitialized);
    ASSERT_EQ(Success,
              TimeChoiceForEquivalentUTCTime(generalizedTimeDER, value));
    EXPECT_EQ(expectedValue, value);
  }
}

template <uint16_t LENGTH>
void
ExpectBadTime(const uint8_t (&generalizedTimeDER)[LENGTH])
{
  
  {
    Input input(generalizedTimeDER);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Result::ERROR_INVALID_DER_TIME, GeneralizedTime(reader, value));
  }

  
  {
    Input input(generalizedTimeDER);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Result::ERROR_INVALID_DER_TIME, TimeChoice(reader, value));
  }

  
  {
    Time value(Time::uninitialized);
    ASSERT_EQ(Result::ERROR_INVALID_DER_TIME,
              TimeChoiceForEquivalentUTCTime(generalizedTimeDER, value));
  }
}


TEST_F(pkixder_universal_types_tests, ValidControl)
{
  const uint8_t GT_DER[] = {
    0x18,                           
    15,                             
    '1', '9', '9', '1', '0', '5', '0', '6', '1', '6', '4', '5', '4', '0', 'Z'
  };
  ExpectGoodTime(YMDHMS(1991, 5, 6, 16, 45, 40), GT_DER);
}

TEST_F(pkixder_universal_types_tests, TimeTimeZoneOffset)
{
  const uint8_t DER_GENERALIZED_TIME_OFFSET[] = {
    0x18,                           
    19,                             
    '1', '9', '9', '1', '0', '5', '0', '6', '1', '6', '4', '5', '4', '0', '-',
    '0', '7', '0', '0'
  };
  ExpectBadTime(DER_GENERALIZED_TIME_OFFSET);
}

TEST_F(pkixder_universal_types_tests, TimeInvalidZeroLength)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_ZERO_LENGTH[] = {
    0x18,                           
    0x00                            
  };

  Time value(Time::uninitialized);

  
  Input gtBuf(DER_GENERALIZED_TIME_INVALID_ZERO_LENGTH);
  Reader gt(gtBuf);
  ASSERT_EQ(Result::ERROR_INVALID_DER_TIME, GeneralizedTime(gt, value));

  
  Input tc_gt_buf(DER_GENERALIZED_TIME_INVALID_ZERO_LENGTH);
  Reader tc_gt(tc_gt_buf);
  ASSERT_EQ(Result::ERROR_INVALID_DER_TIME, TimeChoice(tc_gt, value));

  
  const uint8_t DER_UTCTIME_INVALID_ZERO_LENGTH[] = {
    0x17, 
    0x00  
  };
  Input tc_utc_buf(DER_UTCTIME_INVALID_ZERO_LENGTH);
  Reader tc_utc(tc_utc_buf);
  ASSERT_EQ(Result::ERROR_INVALID_DER_TIME, TimeChoice(tc_utc, value));
}


TEST_F(pkixder_universal_types_tests, TimeInvalidLocal)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_LOCAL[] = {
    0x18,                           
    14,                             
    '1', '9', '9', '1', '0', '5', '0', '6', '1', '6', '4', '5', '4', '0'
  };
  ExpectBadTime(DER_GENERALIZED_TIME_INVALID_LOCAL);
}


TEST_F(pkixder_universal_types_tests, TimeInvalidTruncated)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_TRUNCATED[] = {
    0x18,                           
    12,                             
    '1', '9', '9', '1', '0', '5', '0', '6', '1', '6', '4', '5'
  };
  ExpectBadTime(DER_GENERALIZED_TIME_INVALID_TRUNCATED);
}

TEST_F(pkixder_universal_types_tests, TimeNoSeconds)
{
  const uint8_t DER_GENERALIZED_TIME_NO_SECONDS[] = {
    0x18,                           
    13,                             
    '1', '9', '9', '1', '0', '5', '0', '6', '1', '6', '4', '5', 'Z'
  };
  ExpectBadTime(DER_GENERALIZED_TIME_NO_SECONDS);
}

TEST_F(pkixder_universal_types_tests, TimeInvalidPrefixedYear)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_PREFIXED_YEAR[] = {
    0x18,                           
    16,                             
    ' ', '1', '9', '9', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', 'Z'
  };
  ExpectBadTime(DER_GENERALIZED_TIME_INVALID_PREFIXED_YEAR);
}

TEST_F(pkixder_universal_types_tests, TimeTooManyDigits)
{
  const uint8_t DER_GENERALIZED_TIME_TOO_MANY_DIGITS[] = {
    0x18,                           
    16,                             
    '1', '1', '1', '1', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', 'Z'
  };
  ExpectBadTime(DER_GENERALIZED_TIME_TOO_MANY_DIGITS);
}



TEST_F(pkixder_universal_types_tests, GeneralizedTimeYearValidRange)
{
  
  
  

  for (uint16_t i = 1970; i <= 9999; ++i) {
    const uint8_t DER[] = {
      0x18,                           
      15,                             
      TWO_CHARS(i / 100), TWO_CHARS(i % 100), 
      '1', '2', '3', '1', 
      '2', '3', '5', '9', '5', '9', 'Z' 
    };

    Time expectedValue = YMDHMS(i, 12, 31, 23, 59, 59);

    
    
    

    
    {
      Input input(DER);
      Reader reader(input);
      Time value(Time::uninitialized);
      ASSERT_EQ(Success, GeneralizedTime(reader, value));
      EXPECT_EQ(expectedValue, value);
    }

    
    {
      Input input(DER);
      Reader reader(input);
      Time value(Time::uninitialized);
      ASSERT_EQ(Success, TimeChoice(reader, value));
      EXPECT_EQ(expectedValue, value);
    }

    
    if (i <= 2049) {
      Time value(Time::uninitialized);
      ASSERT_EQ(Success, TimeChoiceForEquivalentUTCTime(DER, value));
      EXPECT_EQ(expectedValue, value);
    }
  }
}



TEST_F(pkixder_universal_types_tests, TimeYearInvalid1969)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '1', '9', '6', '9', '1', '2', '3', '1', 
    '2', '3', '5', '9', '5', '9', 'Z' 
  };
  ExpectBadTime(DER);
}

static const uint8_t DAYS_IN_MONTH[] = {
  0,  
  31, 
  28, 
  31, 
  30, 
  31, 
  30, 
  31, 
  31, 
  30, 
  31, 
  30, 
  31, 
};

TEST_F(pkixder_universal_types_tests, TimeMonthDaysValidRange)
{
  for (uint16_t month = 1; month <= 12; ++month) {
    for (uint8_t day = 1; day <= DAYS_IN_MONTH[month]; ++day) {
      const uint8_t DER[] = {
        0x18,                           
        15,                             
        '2', '0', '1', '5', TWO_CHARS(month), TWO_CHARS(day), 
        '1', '6', '4', '5', '4', '0', 'Z' 
      };
      ExpectGoodTime(YMDHMS(2015, month, day, 16, 45, 40), DER);
    }
  }
}

TEST_F(pkixder_universal_types_tests, TimeMonthInvalid0)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '0', '1', '5', '0', '0', '1', '5', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };
  ExpectBadTime(DER);
}

TEST_F(pkixder_universal_types_tests, TimeMonthInvalid13)
{
  const uint8_t DER_GENERALIZED_TIME_13TH_MONTH[] = {
    0x18,                           
    15,                             
    '1', '9', '9', '1', 
    '1', '3', 
    '0', '6', '1', '6', '4', '5', '4', '0', 'Z'
  };
  ExpectBadTime(DER_GENERALIZED_TIME_13TH_MONTH);
}

TEST_F(pkixder_universal_types_tests, TimeDayInvalid0)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '0', '1', '5', '0', '1', '0', '0', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };
  ExpectBadTime(DER);
}

TEST_F(pkixder_universal_types_tests, TimeMonthDayInvalidPastEndOfMonth)
{
  for (int16_t month = 1; month <= 12; ++month) {
    const uint8_t DER[] = {
      0x18,                           
      15,                             
      '1', '9', '9', '1', 
      TWO_CHARS(month), 
      TWO_CHARS(1 + (month == 2 ? 29 : DAYS_IN_MONTH[month])), 
      '1', '6', '4', '5', '4', '0', 'Z' 
    };
    ExpectBadTime(DER);
  }
}

TEST_F(pkixder_universal_types_tests, TimeMonthFebLeapYear2016)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '0', '1', '6', '0', '2', '2', '9', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };
  ExpectGoodTime(YMDHMS(2016, 2, 29, 16, 45, 40), DER);
}

TEST_F(pkixder_universal_types_tests, TimeMonthFebLeapYear2000)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '0', '0', '0', '0', '2', '2', '9', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };
  ExpectGoodTime(YMDHMS(2000, 2, 29, 16, 45, 40), DER);
}

TEST_F(pkixder_universal_types_tests, TimeMonthFebLeapYear2400)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '4', '0', '0', '0', '2', '2', '9', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };

  

  Time expectedValue = YMDHMS(2400, 2, 29, 16, 45, 40);

  
  {
    Input input(DER);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Success, GeneralizedTime(reader, value));
    EXPECT_EQ(expectedValue, value);
  }

  
  {
    Input input(DER);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Success, TimeChoice(reader, value));
    EXPECT_EQ(expectedValue, value);
  }
}

TEST_F(pkixder_universal_types_tests, TimeMonthFebNotLeapYear2014)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '0', '1', '4', '0', '2', '2', '9', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };
  ExpectBadTime(DER);
}

TEST_F(pkixder_universal_types_tests, TimeMonthFebNotLeapYear2100)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '1', '0', '0', '0', '2', '2', '9', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };

  

  
  {
    Input input(DER);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Result::ERROR_INVALID_DER_TIME, GeneralizedTime(reader, value));
  }

  
  {
    Input input(DER);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Result::ERROR_INVALID_DER_TIME, TimeChoice(reader, value));
  }
}

TEST_F(pkixder_universal_types_tests, TimeHoursValidRange)
{
  for (uint8_t i = 0; i <= 23; ++i) {
    const uint8_t DER[] = {
      0x18,                           
      15,                             
      '2', '0', '1', '2', '0', '6', '3', '0', 
      TWO_CHARS(i), '5', '9', '0', '1', 'Z' 
    };
    ExpectGoodTime(YMDHMS(2012, 6, 30, i, 59, 1), DER);
  }
}

TEST_F(pkixder_universal_types_tests, TimeHoursInvalid_24_00_00)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '0', '1', '2', '0', '6', '3', '0', 
    '2', '4', '0', '0', '0', '0', 'Z' 
  };
  ExpectBadTime(DER);
}

TEST_F(pkixder_universal_types_tests, TimeMinutesValidRange)
{
  for (uint8_t i = 0; i <= 59; ++i) {
    const uint8_t DER[] = {
      0x18,                           
      15,                             
      '2', '0', '1', '2', '0', '6', '3', '0', 
      '2', '3', TWO_CHARS(i), '0', '1', 'Z' 
    };
    ExpectGoodTime(YMDHMS(2012, 6, 30, 23, i, 1), DER);
  }
}

TEST_F(pkixder_universal_types_tests, TimeMinutesInvalid60)
{
  const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '0', '1', '2', '0', '6', '3', '0', 
    '2', '3', '6', '0', '5', '9', 'Z' 
  };
  ExpectBadTime(DER);
}

TEST_F(pkixder_universal_types_tests, TimeSecondsValidRange)
{
  for (uint8_t i = 0; i <= 59; ++i) {
    const uint8_t DER[] = {
      0x18,                           
      15,                             
      '2', '0', '1', '2', '0', '6', '3', '0', 
      '2', '3', '5', '9', TWO_CHARS(i), 'Z' 
    };
    ExpectGoodTime(YMDHMS(2012, 6, 30, 23, 59, i), DER);
  }
}


TEST_F(pkixder_universal_types_tests, TimeSecondsInvalid60)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '0', '1', '2', '0', '6', '3', '0', 
    '2', '3', '5', '9', '6', '0', 'Z' 
  };
  ExpectBadTime(DER);
}


TEST_F(pkixder_universal_types_tests, TimeSecondsInvalid61)
{
  static const uint8_t DER[] = {
    0x18,                           
    15,                             
    '2', '0', '1', '2', '0', '6', '3', '0', 
    '2', '3', '5', '9', '6', '1', 'Z' 
  };
  ExpectBadTime(DER);
}

TEST_F(pkixder_universal_types_tests, TimeInvalidZulu)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_ZULU[] = {
    0x18,                           
    15,                             
    '2', '0', '1', '2', '0', '6', '3', '0', 
    '2', '3', '5', '9', '5', '9', 'z' 
  };
  ExpectBadTime(DER_GENERALIZED_TIME_INVALID_ZULU);
}

TEST_F(pkixder_universal_types_tests, TimeInvalidExtraData)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_EXTRA_DATA[] = {
    0x18,                           
    16,                             
    '2', '0', '1', '2', '0', '6', '3', '0', 
    '2', '3', '5', '9', '5', '9', 'Z', 
    0 
  };
  ExpectBadTime(DER_GENERALIZED_TIME_INVALID_EXTRA_DATA);
}

TEST_F(pkixder_universal_types_tests, TimeInvalidCenturyChar)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_CENTURY_CHAR[] = {
    0x18,                           
    15,                             
    'X', '9', '9', '1', '1', '2', '0', '6', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };

  
  
  

  
  {
    Input input(DER_GENERALIZED_TIME_INVALID_CENTURY_CHAR);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Result::ERROR_INVALID_DER_TIME, GeneralizedTime(reader, value));
  }

  
  {
    Input input(DER_GENERALIZED_TIME_INVALID_CENTURY_CHAR);
    Reader reader(input);
    Time value(Time::uninitialized);
    ASSERT_EQ(Result::ERROR_INVALID_DER_TIME, TimeChoice(reader, value));
  }

  
}

TEST_F(pkixder_universal_types_tests, TimeInvalidYearChar)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_YEAR_CHAR[] = {
    0x18,                           
    15,                             
    '1', '9', '9', 'I', '0', '1', '0', '6', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };
  ExpectBadTime(DER_GENERALIZED_TIME_INVALID_YEAR_CHAR);
}

TEST_F(pkixder_universal_types_tests, GeneralizedTimeInvalidMonthChar)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_MONTH_CHAR[] = {
    0x18,                           
    15,                             
    '1', '9', '9', '1', '0', 'I', '0', '6', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };
  ExpectBadTime(DER_GENERALIZED_TIME_INVALID_MONTH_CHAR);
}

TEST_F(pkixder_universal_types_tests, TimeInvalidDayChar)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_DAY_CHAR[] = {
    0x18,                           
    15,                             
    '1', '9', '9', '1', '0', '1', '0', 'S', 
    '1', '6', '4', '5', '4', '0', 'Z' 
  };
  ExpectBadTime(DER_GENERALIZED_TIME_INVALID_DAY_CHAR);
}

TEST_F(pkixder_universal_types_tests, TimeInvalidFractionalSeconds)
{
  const uint8_t DER_GENERALIZED_TIME_INVALID_FRACTIONAL_SECONDS[] = {
    0x18,                           
    17,                             
    '1', '9', '9', '1', '0', '1', '0', '1', 
    '1', '6', '4', '5', '4', '0', '.', '3', 'Z' 
  };
  ExpectBadTime(DER_GENERALIZED_TIME_INVALID_FRACTIONAL_SECONDS);
}

struct IntegerTestParams
{
  ByteString encoded;
  struct PositiveIntegerParams
  {
    bool isValid;
    Input::size_type significantBytesIfValid;
  } positiveInteger;
  uint8_t smallNonnegativeIntegerValue;
};

class pkixder_universal_types_tests_Integer
  : public ::testing::Test
  , public ::testing::WithParamInterface<IntegerTestParams>
{
};

#define INVALID 0xFF

static const IntegerTestParams INTEGER_TEST_PARAMS[] =
{
  
  { TLV(2, ByteString()), { false, INVALID }, INVALID },
  { TLV(2, "\x00"), { false, INVALID }, 0 },

  
  { TLV(2, "\x01"), { true, 1 }, 1 },
  { TLV(2, "\x02"), { true, 1 }, 2 },
  { TLV(2, "\x7e"), { true, 1 }, 0x7e },
  { TLV(2, "\x7f"), { true, 1 }, 0x7f },

  
  { TLV(2, "\x80"), { false, INVALID }, INVALID },
  { TLV(2, "\x81"), { false, INVALID }, INVALID },
  { TLV(2, "\xFE"), { false, INVALID }, INVALID },
  { TLV(2, "\xFF"), { false, INVALID }, INVALID },

  
  { TLV(2, "\x7F\x00"), { true, 2 }, INVALID },
  { TLV(2, "\x01\x00"), { true, 2 }, INVALID },
  { TLV(2, "\x01\x02"), { true, 2 }, INVALID },

  
  { TLV(2, "\x80\x00"), { false, INVALID }, INVALID },
  { TLV(2, "\x80\x7F"), { false, INVALID }, INVALID },
  { TLV(2, "\x80\x80"), { false, INVALID }, INVALID },
  { TLV(2, "\x80\xFF"), { false, INVALID }, INVALID },

  
  { TLV(2, "\x00\x80"), { true, 1}, INVALID },
  { TLV(2, "\x00\x81"), { true, 1}, INVALID },
  { TLV(2, "\x00\xFF"), { true, 1}, INVALID },

  
  { TLV(2, "\x00\x01"), { false, INVALID }, INVALID },
  { TLV(2, "\x00\x7F"), { false, INVALID }, INVALID },

  
  { TLV(2, "\xFF\x00"), { false, INVALID }, INVALID },
  { TLV(2, "\xFF\x7F"), { false, INVALID }, INVALID },

  
  { TLV(2, "\xFF\x80"), { false, INVALID }, INVALID },
  { TLV(2, "\xFF\xFF"), { false, INVALID }, INVALID },

  
  { TLV(2, 1, ByteString()), { false, INVALID }, INVALID },
  { TLV(2, 3, "\x11\x22" ), { false, INVALID }, INVALID },
  { TLV(2, 4, "\x11\x22" ), { false, INVALID }, INVALID },
  { TLV(2, 2, "\x00" ), { false, INVALID }, INVALID },
  { TLV(2, 2, "\xFF" ), { false, INVALID }, INVALID },
  { TLV(2, 3, "\x00\x80" ), { false, INVALID }, INVALID },
  { TLV(2, 3, "\xFF\x00" ), { false, INVALID }, INVALID },

  
  { TLV(2, 4, "\x11\x22\x33\x44"), { true, 4 }, INVALID },
  { TLV(2,
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"

        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"
        "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x00"),
    { true, 256 }, INVALID },
};

TEST_P(pkixder_universal_types_tests_Integer, Integer)
{
  const IntegerTestParams& params(GetParam());
  Input input;
  ASSERT_EQ(Success, input.Init(params.encoded.data(),
                                params.encoded.length()));
  Reader reader(input);
  Result expectedResult = params.smallNonnegativeIntegerValue != INVALID
                        ? Success
                        : Result::ERROR_BAD_DER;
  uint8_t value;
  ASSERT_EQ(expectedResult, der::Integer(reader, value));
  if (expectedResult == Success) {
    ASSERT_EQ(params.smallNonnegativeIntegerValue, value);
    ASSERT_TRUE(reader.AtEnd());
  }
}

TEST_P(pkixder_universal_types_tests_Integer,
       PositiveInteger_without_significantBytes)
{
  const IntegerTestParams& params(GetParam());
  Input input;
  ASSERT_EQ(Success, input.Init(params.encoded.data(),
                                params.encoded.length()));
  Reader reader(input);
  Result expectedResult = params.positiveInteger.isValid
                        ? Success
                        : Result::ERROR_BAD_DER;
  Input value;
  ASSERT_EQ(expectedResult, der::PositiveInteger(reader, value));
  if (expectedResult == Success) {
    Reader anotherReader(input);
    Input expectedValue;
    ASSERT_EQ(Success, ExpectTagAndGetValue(anotherReader,
                                            der::INTEGER, expectedValue));
    ASSERT_TRUE(InputsAreEqual(expectedValue, value));
    ASSERT_TRUE(reader.AtEnd());
  }
}

TEST_P(pkixder_universal_types_tests_Integer,
       PositiveInteger_with_significantBytes)
{
  const IntegerTestParams& params(GetParam());
  Input input;
  ASSERT_EQ(Success, input.Init(params.encoded.data(),
                                params.encoded.length()));
  Reader reader(input);
  Result expectedResult = params.positiveInteger.isValid
                        ? Success
                        : Result::ERROR_BAD_DER;
  Input value;
  Input::size_type significantBytes = INVALID;
  ASSERT_EQ(expectedResult, der::PositiveInteger(reader, value,
                                                 &significantBytes));
  if (expectedResult == Success) {
    ASSERT_NE(INVALID, params.positiveInteger.significantBytesIfValid);
    ASSERT_EQ(params.positiveInteger.significantBytesIfValid,
              significantBytes);

    Reader anotherReader(input);
    Input expectedValue;
    ASSERT_EQ(Success, ExpectTagAndGetValue(anotherReader,
                                            der::INTEGER, expectedValue));
    ASSERT_TRUE(InputsAreEqual(expectedValue, value));
    ASSERT_TRUE(reader.AtEnd());
  }
}

#undef INVALID

INSTANTIATE_TEST_CASE_P(pkixder_universal_types_tests_Integer,
                        pkixder_universal_types_tests_Integer,
                        testing::ValuesIn(INTEGER_TEST_PARAMS));

TEST_F(pkixder_universal_types_tests, OptionalIntegerSupportedDefault)
{
  
  
  Input input(DER_BOOLEAN_TRUE);
  Reader reader(input);

  long value = 1;
  ASSERT_EQ(Success, OptionalInteger(reader, -1, value));
  ASSERT_EQ(-1, value);
  bool boolValue;
  ASSERT_EQ(Success, Boolean(reader, boolValue));
}

TEST_F(pkixder_universal_types_tests, OptionalIntegerUnsupportedDefault)
{
  
  
  Input input(DER_BOOLEAN_TRUE);
  Reader reader(input);

  long value;
  ASSERT_EQ(Result::FATAL_ERROR_INVALID_ARGS, OptionalInteger(reader, 0, value));
}

TEST_F(pkixder_universal_types_tests, OptionalIntegerSupportedDefaultAtEnd)
{
  static const uint8_t dummy = 1;
  Input input;
  ASSERT_EQ(Success, input.Init(&dummy, 0));
  Reader reader(input);

  long value = 1;
  ASSERT_EQ(Success, OptionalInteger(reader, -1, value));
  ASSERT_EQ(-1, value);
}

TEST_F(pkixder_universal_types_tests, OptionalIntegerNonDefaultValue)
{
  static const uint8_t DER[] = {
    0x02, 
    0x01, 
    0x00
  };
  Input input(DER);
  Reader reader(input);

  long value = 2;
  ASSERT_EQ(Success, OptionalInteger(reader, -1, value));
  ASSERT_EQ(0, value);
  ASSERT_TRUE(reader.AtEnd());
}

TEST_F(pkixder_universal_types_tests, Null)
{
  const uint8_t DER_NUL[] = {
    0x05,
    0x00
  };
  Input input(DER_NUL);
  Reader reader(input);

  ASSERT_EQ(Success, Null(reader));
}

TEST_F(pkixder_universal_types_tests, NullWithBadLength)
{
  const uint8_t DER_NULL_BAD_LENGTH[] = {
    0x05,
    0x01,
    0x00
  };
  Input input(DER_NULL_BAD_LENGTH);
  Reader reader(input);

  ASSERT_EQ(Result::ERROR_BAD_DER, Null(reader));
}

TEST_F(pkixder_universal_types_tests, OID)
{
  const uint8_t DER_VALID_OID[] = {
    0x06,
    0x09,
    0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };
  Input input(DER_VALID_OID);
  Reader reader(input);

  const uint8_t expectedOID[] = {
    0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };

  ASSERT_EQ(Success, OID(reader, expectedOID));
}
