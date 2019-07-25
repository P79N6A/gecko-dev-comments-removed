





#include "gtest/gtest.h"
#include "Preprocessor.h"
#include "Token.h"

#if GTEST_HAS_COMBINE

typedef std::tr1::tuple<const char*, char> IntegerParams;
class IntegerTest : public testing::TestWithParam<IntegerParams>
{
};

TEST_P(IntegerTest, IntegerIdentified)
{
    std::string str(std::tr1::get<0>(GetParam()));  
    str.push_back(std::tr1::get<1>(GetParam()));  
    const char* cstr = str.c_str();

    pp::Token token;
    pp::Preprocessor preprocessor;
    ASSERT_TRUE(preprocessor.init(1, &cstr, 0));
    EXPECT_EQ(pp::Token::CONST_INT, preprocessor.lex(&token));
    EXPECT_EQ(pp::Token::CONST_INT, token.type);
    EXPECT_STREQ(cstr, token.value.c_str());
}

INSTANTIATE_TEST_CASE_P(DecimalInteger,
                        IntegerTest,
                        testing::Combine(testing::Values(""),
                                         testing::Range('0', '9')));

INSTANTIATE_TEST_CASE_P(OctalInteger,
                        IntegerTest,
                        testing::Combine(testing::Values("0"),
                                         testing::Range('0', '7')));

INSTANTIATE_TEST_CASE_P(HexadecimalInteger_0_9,
                        IntegerTest,
                        testing::Combine(testing::Values("0x"),
                                         testing::Range('0', '9')));

INSTANTIATE_TEST_CASE_P(HexadecimalInteger_a_f,
                        IntegerTest,
                        testing::Combine(testing::Values("0x"),
                                         testing::Range('a', 'f')));

INSTANTIATE_TEST_CASE_P(HexadecimalInteger_A_F,
                        IntegerTest,
                        testing::Combine(testing::Values("0x"),
                                         testing::Range('A', 'F')));

static void PreprocessAndVerifyFloat(const char* str)
{
    pp::Token token;
    pp::Preprocessor preprocessor;
    ASSERT_TRUE(preprocessor.init(1, &str, 0));
    EXPECT_EQ(pp::Token::CONST_FLOAT, preprocessor.lex(&token));
    EXPECT_EQ(pp::Token::CONST_FLOAT, token.type);
    EXPECT_STREQ(str, token.value.c_str());
}

typedef std::tr1::tuple<char, char, const char*, char> FloatScientificParams;
class FloatScientificTest : public testing::TestWithParam<FloatScientificParams>
{
};


TEST_P(FloatScientificTest, FloatIdentified)
{
    std::string str;
    str.push_back(std::tr1::get<0>(GetParam()));  
    str.push_back(std::tr1::get<1>(GetParam()));  
    str.append(std::tr1::get<2>(GetParam()));  
    str.push_back(std::tr1::get<3>(GetParam()));  

    SCOPED_TRACE("FloatScientificTest");
    PreprocessAndVerifyFloat(str.c_str());
}

INSTANTIATE_TEST_CASE_P(FloatScientific,
                        FloatScientificTest,
                        testing::Combine(testing::Range('0', '9'),
                                         testing::Values('e', 'E'),
                                         testing::Values("", "+", "-"),
                                         testing::Range('0', '9')));

typedef std::tr1::tuple<char, char> FloatFractionParams;
class FloatFractionTest : public testing::TestWithParam<FloatFractionParams>
{
};


TEST_P(FloatFractionTest, FloatIdentified)
{
    std::string str;

    char significand = std::tr1::get<0>(GetParam());
    if (significand != '\0')
        str.push_back(significand);

    str.push_back('.');

    char fraction = std::tr1::get<1>(GetParam());
    if (fraction != '\0')
        str.push_back(fraction);

    SCOPED_TRACE("FloatFractionTest");
    PreprocessAndVerifyFloat(str.c_str());
}

INSTANTIATE_TEST_CASE_P(FloatFraction_X_X,
                        FloatFractionTest,
                        testing::Combine(testing::Range('0', '9'),
                                         testing::Range('0', '9')));

INSTANTIATE_TEST_CASE_P(FloatFraction_0_X,
                        FloatFractionTest,
                        testing::Combine(testing::Values('\0'),
                                         testing::Range('0', '9')));

INSTANTIATE_TEST_CASE_P(FloatFraction_X_0,
                        FloatFractionTest,
                        testing::Combine(testing::Range('0', '9'),
                                         testing::Values('\0')));

#endif  




TEST(FloatFractionScientificTest, FloatIdentified)
{
    SCOPED_TRACE("FloatFractionScientificTest");
    PreprocessAndVerifyFloat("0.1e+2");
}
