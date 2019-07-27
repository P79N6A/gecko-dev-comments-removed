























#include <limits>
#include <vector>
#include <gtest/gtest.h>

#include "pkix/bind.h"
#include "pkixder.h"
#include "pkixtestutil.h"
#include "stdint.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::der;
using namespace mozilla::pkix::test;
using namespace std;

namespace {

class pkixder_universal_types_tests : public ::testing::Test { };

TEST_F(pkixder_universal_types_tests, BooleanTrue01)
{
  const uint8_t DER_BOOLEAN_TRUE_01[] = {
    0x01,                       
    0x01,                       
    0x01                        
  };

  Input input;
  ASSERT_EQ(Success,
            input.Init(DER_BOOLEAN_TRUE_01, sizeof DER_BOOLEAN_TRUE_01));

  bool value = false;
  ASSERT_EQ(Result::ERROR_BAD_DER, Boolean(input, value));
}

TEST_F(pkixder_universal_types_tests, BooleanTrue42)
{
  const uint8_t DER_BOOLEAN_TRUE_42[] = {
    0x01,                       
    0x01,                       
    0x42                        
  };

  Input input;
  ASSERT_EQ(Success,
            input.Init(DER_BOOLEAN_TRUE_42, sizeof DER_BOOLEAN_TRUE_42));

  bool value = false;
  ASSERT_EQ(Result::ERROR_BAD_DER, Boolean(input, value));
}

static const uint8_t DER_BOOLEAN_TRUE[] = {
  0x01,                       
  0x01,                       
  0xff                        
};

TEST_F(pkixder_universal_types_tests, BooleanTrueFF)
{
  Input input;
  ASSERT_EQ(Success,
            input.Init(DER_BOOLEAN_TRUE, sizeof DER_BOOLEAN_TRUE));

  bool value = false;
  ASSERT_EQ(Success, Boolean(input, value));
  ASSERT_TRUE(value);
}

TEST_F(pkixder_universal_types_tests, BooleanFalse)
{
  const uint8_t DER_BOOLEAN_FALSE[] = {
    0x01,                       
    0x01,                       
    0x00                        
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_BOOLEAN_FALSE, sizeof DER_BOOLEAN_FALSE));

  bool value = true;
  ASSERT_EQ(Success, Boolean(input, value));
  ASSERT_FALSE(value);
}

TEST_F(pkixder_universal_types_tests, BooleanInvalidLength)
{
  const uint8_t DER_BOOLEAN_INVALID_LENGTH[] = {
    0x01,                       
    0x02,                       
    0x42, 0x42                  
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_BOOLEAN_INVALID_LENGTH,
                                sizeof DER_BOOLEAN_INVALID_LENGTH));

  bool value = true;
  ASSERT_EQ(Result::ERROR_BAD_DER, Boolean(input, value));
}

TEST_F(pkixder_universal_types_tests, BooleanInvalidZeroLength)
{
  const uint8_t DER_BOOLEAN_INVALID_ZERO_LENGTH[] = {
    0x01,                       
    0x00                        
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_BOOLEAN_INVALID_ZERO_LENGTH,
                                sizeof DER_BOOLEAN_INVALID_ZERO_LENGTH));

  bool value = true;
  ASSERT_EQ(Result::ERROR_BAD_DER, Boolean(input, value));
}







TEST_F(pkixder_universal_types_tests, OptionalBooleanValidEncodings)
{
  const uint8_t DER_OPTIONAL_BOOLEAN_PRESENT_TRUE[] = {
    0x01,                       
    0x01,                       
    0xff                        
  };

  Input input1;
  ASSERT_EQ(Success, input1.Init(DER_OPTIONAL_BOOLEAN_PRESENT_TRUE,
                                 sizeof DER_OPTIONAL_BOOLEAN_PRESENT_TRUE));
  bool value = false;
  ASSERT_EQ(Success, OptionalBoolean(input1, false, value)) <<
    "Should accept the only valid encoding of a present OPTIONAL BOOLEAN";
  ASSERT_TRUE(value);
  ASSERT_TRUE(input1.AtEnd());

  
  const uint8_t DER_INTEGER_05[] = {
    0x02,                       
    0x01,                       
    0x05
  };

  Input input2;
  ASSERT_EQ(Success, input2.Init(DER_INTEGER_05, sizeof DER_INTEGER_05));
  value = true;
  ASSERT_EQ(Success, OptionalBoolean(input2, false, value)) <<
    "Should accept a valid encoding of an omitted OPTIONAL BOOLEAN";
  ASSERT_FALSE(value);
  ASSERT_FALSE(input2.AtEnd());

  Input input3;
  ASSERT_EQ(Success, input3.Init(reinterpret_cast<const uint8_t*>(""), 0));
  value = true;
  ASSERT_EQ(Success, OptionalBoolean(input3, false, value)) <<
    "Should accept another valid encoding of an omitted OPTIONAL BOOLEAN";
  ASSERT_FALSE(value);
  ASSERT_TRUE(input3.AtEnd());
}

TEST_F(pkixder_universal_types_tests, OptionalBooleanInvalidEncodings)
{
  const uint8_t DER_OPTIONAL_BOOLEAN_PRESENT_FALSE[] = {
    0x01,                       
    0x01,                       
    0x00                        
  };

  Input input1;
  ASSERT_EQ(Success, input1.Init(DER_OPTIONAL_BOOLEAN_PRESENT_FALSE,
                                 sizeof DER_OPTIONAL_BOOLEAN_PRESENT_FALSE));
  bool value;
  
  
  bool allowInvalidEncodings = false;
  ASSERT_EQ(Result::ERROR_BAD_DER,
            OptionalBoolean(input1, allowInvalidEncodings, value)) <<
    "Should reject an invalid encoding of present OPTIONAL BOOLEAN";

  Input input2;
  ASSERT_EQ(Success, input2.Init(DER_OPTIONAL_BOOLEAN_PRESENT_FALSE,
                                 sizeof DER_OPTIONAL_BOOLEAN_PRESENT_FALSE));
  value = true;
  
  
  allowInvalidEncodings = true;
  ASSERT_EQ(Success, OptionalBoolean(input2, allowInvalidEncodings, value)) <<
    "Should now accept an invalid encoding of present OPTIONAL BOOLEAN";
  ASSERT_FALSE(value);
  ASSERT_TRUE(input2.AtEnd());

  const uint8_t DER_OPTIONAL_BOOLEAN_PRESENT_42[] = {
    0x01,                       
    0x01,                       
    0x42                        
  };

  Input input3;
  ASSERT_EQ(Success, input3.Init(DER_OPTIONAL_BOOLEAN_PRESENT_42,
                                 sizeof DER_OPTIONAL_BOOLEAN_PRESENT_42));
  
  
  ASSERT_EQ(Result::ERROR_BAD_DER,
            OptionalBoolean(input3, allowInvalidEncodings, value)) <<
    "Should reject another invalid encoding of present OPTIONAL BOOLEAN";
}

TEST_F(pkixder_universal_types_tests, Enumerated)
{
  const uint8_t DER_ENUMERATED[] = {
    0x0a,                       
    0x01,                       
    0x42                        
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_ENUMERATED, sizeof DER_ENUMERATED));

  uint8_t value = 0;
  ASSERT_EQ(Success, Enumerated(input, value));
  ASSERT_EQ(0x42, value);
}

TEST_F(pkixder_universal_types_tests, EnumeratedNotShortestPossibleDER)
{
  const uint8_t DER_ENUMERATED[] = {
    0x0a,                       
    0x02,                       
    0x00, 0x01                  
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_ENUMERATED, sizeof DER_ENUMERATED));
  uint8_t value = 0;
  ASSERT_EQ(Result::ERROR_BAD_DER, Enumerated(input, value));
}

TEST_F(pkixder_universal_types_tests, EnumeratedOutOfAcceptedRange)
{
  
  
  
  
  const uint8_t DER_ENUMERATED_INVALID_LENGTH[] = {
    0x0a,                       
    0x02,                       
    0x12, 0x34                  
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_ENUMERATED_INVALID_LENGTH,
                                sizeof DER_ENUMERATED_INVALID_LENGTH));

  uint8_t value = 0;
  ASSERT_EQ(Result::ERROR_BAD_DER, Enumerated(input, value));
}

TEST_F(pkixder_universal_types_tests, EnumeratedInvalidZeroLength)
{
  const uint8_t DER_ENUMERATED_INVALID_ZERO_LENGTH[] = {
    0x0a,                       
    0x00                        
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_ENUMERATED_INVALID_ZERO_LENGTH,
                                sizeof DER_ENUMERATED_INVALID_ZERO_LENGTH));

  uint8_t value = 0;
  ASSERT_EQ(Result::ERROR_BAD_DER, Enumerated(input, value));
}























#define TWO_CHARS(t) static_cast<uint8_t>('0' + ((t) / 10u)), \
                     static_cast<uint8_t>('0' + ((t) % 10u))


template <uint16_t LENGTH>
Result
TimeChoiceForEquivalentUTCTime(const uint8_t (&generalizedTimeDER)[LENGTH],
                                PRTime& value)
{
  static_assert(LENGTH >= 4,
                "TimeChoiceForEquivalentUTCTime input too small");
  uint8_t utcTimeDER[LENGTH - 2];
  utcTimeDER[0] = 0x17; 
  utcTimeDER[1] = LENGTH - 1 - 1 - 2;
  
  for (size_t i = 2; i < LENGTH - 2; ++i) {
    utcTimeDER[i] = generalizedTimeDER[i + 2];
  }

  Input input;
  Result rv = input.Init(utcTimeDER, sizeof utcTimeDER);
  EXPECT_EQ(Success, rv);
  if (rv != Success) {
    return rv;
  }
  return TimeChoice(input, value);
}

template <uint16_t LENGTH>
void
ExpectGoodTime(PRTime expectedValue,
               const uint8_t (&generalizedTimeDER)[LENGTH])
{
  
  {
    Input input;
    ASSERT_EQ(Success, input.Init(generalizedTimeDER, LENGTH));
    PRTime value = 0;
    ASSERT_EQ(Success, GeneralizedTime(input, value));
    EXPECT_EQ(expectedValue, value);
  }

  
  {
    Input input;
    ASSERT_EQ(Success, input.Init(generalizedTimeDER, LENGTH));
    PRTime value = 0;
    ASSERT_EQ(Success, TimeChoice(input, value));
    EXPECT_EQ(expectedValue, value);
  }

  
  {
    PRTime value = 0;
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
    Input input;
    ASSERT_EQ(Success, input.Init(generalizedTimeDER, LENGTH));
    PRTime value;
    ASSERT_EQ(Result::ERROR_INVALID_TIME, GeneralizedTime(input, value));
  }

  
  {
    Input input;
    ASSERT_EQ(Success, input.Init(generalizedTimeDER, LENGTH));
    PRTime value;
    ASSERT_EQ(Result::ERROR_INVALID_TIME, TimeChoice(input, value));
  }

  
  {
    PRTime value;
    ASSERT_EQ(Result::ERROR_INVALID_TIME,
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

  PRTime value;

  
  Input gt;
  ASSERT_EQ(Success,
            gt.Init(DER_GENERALIZED_TIME_INVALID_ZERO_LENGTH,
                    sizeof DER_GENERALIZED_TIME_INVALID_ZERO_LENGTH));
  ASSERT_EQ(Result::ERROR_INVALID_TIME, GeneralizedTime(gt, value));

  
  Input tc_gt;
  ASSERT_EQ(Success,
            tc_gt.Init(DER_GENERALIZED_TIME_INVALID_ZERO_LENGTH,
                       sizeof DER_GENERALIZED_TIME_INVALID_ZERO_LENGTH));
  ASSERT_EQ(Result::ERROR_INVALID_TIME, TimeChoice(tc_gt, value));

  
  const uint8_t DER_UTCTIME_INVALID_ZERO_LENGTH[] = {
    0x17, 
    0x00  
  };
  Input tc_utc;
  ASSERT_EQ(Success, tc_utc.Init(DER_UTCTIME_INVALID_ZERO_LENGTH,
                                 sizeof DER_UTCTIME_INVALID_ZERO_LENGTH));
  ASSERT_EQ(Result::ERROR_INVALID_TIME, TimeChoice(tc_utc, value));
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

    PRTime expectedValue = YMDHMS(i, 12, 31, 23, 59, 59);

    
    
    

    
    {
      Input input;
      ASSERT_EQ(Success, input.Init(DER, sizeof(DER)));
      PRTime value = 0;
      ASSERT_EQ(Success, GeneralizedTime(input, value));
      EXPECT_EQ(expectedValue, value);
    }

    
    {
      Input input;
      ASSERT_EQ(Success, input.Init(DER, sizeof(DER)));
      PRTime value = 0;
      ASSERT_EQ(Success, TimeChoice(input, value));
      EXPECT_EQ(expectedValue, value);
    }

    
    if (i <= 2049) {
      Input input;
      PRTime value = 0;
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
  for (uint8_t month = 1; month <= 12; ++month) {
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
  for (uint8_t month = 1; month <= 12; ++month) {
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

  

  PRTime expectedValue = YMDHMS(2400, 2, 29, 16, 45, 40);

  
  {
    Input input;
    ASSERT_EQ(Success, input.Init(DER, sizeof(DER)));
    PRTime value = 0;
    ASSERT_EQ(Success, GeneralizedTime(input, value));
    EXPECT_EQ(expectedValue, value);
  }

  
  {
    Input input;
    ASSERT_EQ(Success, input.Init(DER, sizeof(DER)));
    PRTime value = 0;
    ASSERT_EQ(Success, TimeChoice(input, value));
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
    Input input;
    ASSERT_EQ(Success, input.Init(DER, sizeof(DER)));
    PRTime value;
    ASSERT_EQ(Result::ERROR_INVALID_TIME, GeneralizedTime(input, value));
  }

  
  {
    Input input;
    ASSERT_EQ(Success, input.Init(DER, sizeof(DER)));
    PRTime value;
    ASSERT_EQ(Result::ERROR_INVALID_TIME, TimeChoice(input, value));
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
    Input input;
    ASSERT_EQ(Success,
              input.Init(DER_GENERALIZED_TIME_INVALID_CENTURY_CHAR,
                         sizeof DER_GENERALIZED_TIME_INVALID_CENTURY_CHAR));
    PRTime value = 0;
    ASSERT_EQ(Result::ERROR_INVALID_TIME, GeneralizedTime(input, value));
  }

  
  {
    Input input;
    ASSERT_EQ(Success,
              input.Init(DER_GENERALIZED_TIME_INVALID_CENTURY_CHAR,
                         sizeof DER_GENERALIZED_TIME_INVALID_CENTURY_CHAR));
    PRTime value = 0;
    ASSERT_EQ(Result::ERROR_INVALID_TIME, TimeChoice(input, value));
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

TEST_F(pkixder_universal_types_tests, Integer_0_127)
{
  for (uint8_t i = 0; i <= 127; ++i) {
    const uint8_t DER[] = {
      0x02, 
      0x01, 
      i,    
    };

    Input input;
    ASSERT_EQ(Success, input.Init(DER, sizeof DER));

    uint8_t value = i + 1; 
    ASSERT_EQ(Success, Integer(input, value));
    ASSERT_EQ(i, value);
  }
}

TEST_F(pkixder_universal_types_tests, Integer_Negative1)
{
  
  

  static const uint8_t DER[] = {
    0x02, 
    0x01, 
    0xff, 
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER, sizeof DER));

  uint8_t value;
  ASSERT_EQ(Result::ERROR_BAD_DER, Integer(input, value));
}

TEST_F(pkixder_universal_types_tests, Integer_Negative128)
{
  
  

  static const uint8_t DER[] = {
    0x02, 
    0x01, 
    0x80, 
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER, sizeof DER));

  uint8_t value;
  ASSERT_EQ(Result::ERROR_BAD_DER, Integer(input, value));
}

TEST_F(pkixder_universal_types_tests, Integer_128)
{
  
  

  static const uint8_t DER[] = {
    0x02, 
    0x02, 
    0x00, 0x80 
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER, sizeof DER));

  uint8_t value;
  ASSERT_EQ(Result::ERROR_BAD_DER, Integer(input, value));
}

TEST_F(pkixder_universal_types_tests, Integer11223344)
{
  
  

  static const uint8_t DER[] = {
    0x02,                       
    0x04,                       
    0x11, 0x22, 0x33, 0x44      
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER, sizeof DER));

  uint8_t value;
  ASSERT_EQ(Result::ERROR_BAD_DER, Integer(input, value));
}

TEST_F(pkixder_universal_types_tests, IntegerTruncatedOneByte)
{
  const uint8_t DER_INTEGER_TRUNCATED[] = {
    0x02,                       
    0x01,                       
    
  };

  Input input;
  ASSERT_EQ(Success,
            input.Init(DER_INTEGER_TRUNCATED, sizeof DER_INTEGER_TRUNCATED));

  uint8_t value;
  ASSERT_EQ(Result::ERROR_BAD_DER, Integer(input, value));
}

TEST_F(pkixder_universal_types_tests, IntegerTruncatedLarge)
{
  const uint8_t DER_INTEGER_TRUNCATED[] = {
    0x02,                       
    0x04,                       
    0x11, 0x22                  
    
  };

  Input input;
  ASSERT_EQ(Success,
            input.Init(DER_INTEGER_TRUNCATED, sizeof DER_INTEGER_TRUNCATED));

  uint8_t value;
  ASSERT_EQ(Result::ERROR_BAD_DER, Integer(input, value));
}

TEST_F(pkixder_universal_types_tests, IntegerZeroLength)
{
  const uint8_t DER_INTEGER_ZERO_LENGTH[] = {
    0x02,                       
    0x00                        
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_INTEGER_ZERO_LENGTH,
                                sizeof DER_INTEGER_ZERO_LENGTH));
  uint8_t value;
  ASSERT_EQ(Result::ERROR_BAD_DER, Integer(input, value));
}

TEST_F(pkixder_universal_types_tests, IntegerOverlyLong1)
{
  const uint8_t DER_INTEGER_OVERLY_LONG1[] = {
    0x02,                       
    0x02,                       
    0x00, 0x01                  
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_INTEGER_OVERLY_LONG1,
                                sizeof DER_INTEGER_OVERLY_LONG1));
  uint8_t value;
  ASSERT_EQ(Result::ERROR_BAD_DER, Integer(input, value));
}

TEST_F(pkixder_universal_types_tests, IntegerOverlyLong2)
{
  const uint8_t DER_INTEGER_OVERLY_LONG2[] = {
    0x02,                       
    0x02,                       
    0xff, 0x80                  
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_INTEGER_OVERLY_LONG2,
                                sizeof DER_INTEGER_OVERLY_LONG2));
  uint8_t value;
  ASSERT_EQ(Result::ERROR_BAD_DER, Integer(input, value));
}

TEST_F(pkixder_universal_types_tests, OptionalIntegerSupportedDefault)
{
  
  
  Input input;
  ASSERT_EQ(Success, input.Init(DER_BOOLEAN_TRUE, sizeof DER_BOOLEAN_TRUE));
  long value = 1;
  ASSERT_EQ(Success, OptionalInteger(input, -1, value));
  ASSERT_EQ(-1, value);
  bool boolValue;
  ASSERT_EQ(Success, Boolean(input, boolValue));
}

TEST_F(pkixder_universal_types_tests, OptionalIntegerUnsupportedDefault)
{
  
  
  Input input;
  ASSERT_EQ(Success, input.Init(DER_BOOLEAN_TRUE, sizeof DER_BOOLEAN_TRUE));
  long value;
  ASSERT_EQ(Result::FATAL_ERROR_INVALID_ARGS, OptionalInteger(input, 0, value));
}

TEST_F(pkixder_universal_types_tests, OptionalIntegerSupportedDefaultAtEnd)
{
  static const uint8_t dummy = 1;

  Input input;
  ASSERT_EQ(Success, input.Init(&dummy, 0));
  long value = 1;
  ASSERT_EQ(Success, OptionalInteger(input, -1, value));
  ASSERT_EQ(-1, value);
}

TEST_F(pkixder_universal_types_tests, OptionalIntegerNonDefaultValue)
{
  static const uint8_t DER[] = {
    0x02, 
    0x01, 
    0x00
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER, sizeof DER));
  long value = 2;
  ASSERT_EQ(Success, OptionalInteger(input, -1, value));
  ASSERT_EQ(0, value);
  ASSERT_TRUE(input.AtEnd());
}

TEST_F(pkixder_universal_types_tests, Null)
{
  const uint8_t DER_NUL[] = {
    0x05,
    0x00
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_NUL, sizeof DER_NUL));
  ASSERT_EQ(Success, Null(input));
}

TEST_F(pkixder_universal_types_tests, NullWithBadLength)
{
  const uint8_t DER_NULL_BAD_LENGTH[] = {
    0x05,
    0x01,
    0x00
  };

  Input input;
  ASSERT_EQ(Success,
            input.Init(DER_NULL_BAD_LENGTH, sizeof DER_NULL_BAD_LENGTH));

  ASSERT_EQ(Result::ERROR_BAD_DER, Null(input));
}

TEST_F(pkixder_universal_types_tests, OID)
{
  const uint8_t DER_VALID_OID[] = {
    0x06,
    0x09,
    0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };

  Input input;
  ASSERT_EQ(Success, input.Init(DER_VALID_OID, sizeof DER_VALID_OID));

  const uint8_t expectedOID[] = {
    0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };

  ASSERT_EQ(Success, OID(input, expectedOID));
}

} 
