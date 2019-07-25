





#include "gtest/gtest.h"
#include "Preprocessor.h"
#include "Token.h"



static const char kSpaceChars[] = {' ', '\t', '\v', '\f'};

#if GTEST_HAS_PARAM_TEST




class SpaceCharTest : public testing::TestWithParam<char>
{
};

TEST_P(SpaceCharTest, SpaceIgnored)
{
    
    const char* identifier = "foo";
    std::string str(1, GetParam());
    str.append(identifier);
    const char* cstr = str.c_str();

    pp::Token token;
    pp::Preprocessor preprocessor;
    ASSERT_TRUE(preprocessor.init(1, &cstr, 0));
    
    EXPECT_EQ(pp::Token::IDENTIFIER, preprocessor.lex(&token));
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_STREQ(identifier, token.value.c_str());
    
    EXPECT_TRUE(token.hasLeadingSpace());
}

INSTANTIATE_TEST_CASE_P(SingleSpaceChar,
                        SpaceCharTest,
                        testing::ValuesIn(kSpaceChars));

#endif  

#if GTEST_HAS_COMBINE




typedef std::tr1::tuple<char, char, char> SpaceStringParams;
class SpaceStringTest : public testing::TestWithParam<SpaceStringParams>
{
};

TEST_P(SpaceStringTest, SpaceIgnored)
{
    
    const char* identifier = "foo";
    std::string str(1, std::tr1::get<0>(GetParam()));
    str.push_back(std::tr1::get<1>(GetParam()));
    str.push_back(std::tr1::get<2>(GetParam()));
    str.append(identifier);
    const char* cstr = str.c_str();

    pp::Token token;
    pp::Preprocessor preprocessor;
    ASSERT_TRUE(preprocessor.init(1, &cstr, 0));
    
    EXPECT_EQ(pp::Token::IDENTIFIER, preprocessor.lex(&token));
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_STREQ(identifier, token.value.c_str());
    
    EXPECT_TRUE(token.hasLeadingSpace());
}

INSTANTIATE_TEST_CASE_P(SpaceCharCombination,
                        SpaceStringTest,
                        testing::Combine(testing::ValuesIn(kSpaceChars),
                                         testing::ValuesIn(kSpaceChars),
                                         testing::ValuesIn(kSpaceChars)));

#endif  




TEST(SpaceTest, LeadingSpace)
{
    const char* str = " foo+ -bar";

    pp::Token token;
    pp::Preprocessor preprocessor;
    ASSERT_TRUE(preprocessor.init(1, &str, 0));

    EXPECT_EQ(pp::Token::IDENTIFIER, preprocessor.lex(&token));
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_STREQ("foo", token.value.c_str());
    EXPECT_TRUE(token.hasLeadingSpace());

    EXPECT_EQ('+', preprocessor.lex(&token));
    EXPECT_EQ('+', token.type);
    EXPECT_FALSE(token.hasLeadingSpace());

    EXPECT_EQ('-', preprocessor.lex(&token));
    EXPECT_EQ('-', token.type);
    EXPECT_TRUE(token.hasLeadingSpace());

    EXPECT_EQ(pp::Token::IDENTIFIER, preprocessor.lex(&token));
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_STREQ("bar", token.value.c_str());
    EXPECT_FALSE(token.hasLeadingSpace());
}
