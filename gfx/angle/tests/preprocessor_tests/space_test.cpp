





#include "PreprocessorTest.h"
#include "Token.h"

class SpaceTest : public PreprocessorTest
{
  protected:
    void expectSpace(const std::string& str)
    {
        const char* cstr = str.c_str();
        ASSERT_TRUE(mPreprocessor.init(1, &cstr, 0));

        pp::Token token;
        
        mPreprocessor.lex(&token);
        EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
        EXPECT_EQ("foo", token.text);
        
        EXPECT_TRUE(token.hasLeadingSpace());
    }
};



static const char kSpaceChars[] = {' ', '\t', '\v', '\f'};




class SpaceCharTest : public SpaceTest,
                      public testing::WithParamInterface<char>
{
};

TEST_P(SpaceCharTest, SpaceIgnored)
{
    
    std::string str(1, GetParam());
    str.append("foo");

    expectSpace(str);
}

INSTANTIATE_TEST_CASE_P(SingleSpaceChar,
                        SpaceCharTest,
                        testing::ValuesIn(kSpaceChars));




typedef std::tr1::tuple<char, char, char> SpaceStringParams;
class SpaceStringTest : public SpaceTest,
                        public testing::WithParamInterface<SpaceStringParams>
{
};

TEST_P(SpaceStringTest, SpaceIgnored)
{
    
    std::string str;
    str.push_back(std::tr1::get<0>(GetParam()));
    str.push_back(std::tr1::get<1>(GetParam()));
    str.push_back(std::tr1::get<2>(GetParam()));
    str.append("foo");

    expectSpace(str);
}

INSTANTIATE_TEST_CASE_P(SpaceCharCombination,
                        SpaceStringTest,
                        testing::Combine(testing::ValuesIn(kSpaceChars),
                                         testing::ValuesIn(kSpaceChars),
                                         testing::ValuesIn(kSpaceChars)));




TEST_F(SpaceTest, LeadingSpace)
{
    const char* str = " foo+ -bar";
    ASSERT_TRUE(mPreprocessor.init(1, &str, 0));

    pp::Token token;
    mPreprocessor.lex(&token);
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_EQ("foo", token.text);
    EXPECT_TRUE(token.hasLeadingSpace());

    mPreprocessor.lex(&token);
    EXPECT_EQ('+', token.type);
    EXPECT_FALSE(token.hasLeadingSpace());

    mPreprocessor.lex(&token);
    EXPECT_EQ('-', token.type);
    EXPECT_TRUE(token.hasLeadingSpace());

    mPreprocessor.lex(&token);
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_EQ("bar", token.text);
    EXPECT_FALSE(token.hasLeadingSpace());
}
