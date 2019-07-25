





#include "PreprocessorTest.h"
#include "Token.h"

#define CLOSED_RANGE(x, y) testing::Range(x, static_cast<char>((y) + 1))

class IdentifierTest : public PreprocessorTest
{
protected:
    void expectIdentifier(const std::string& str)
    {
        const char* cstr = str.c_str();
        ASSERT_TRUE(mPreprocessor.init(1, &cstr, 0));

        pp::Token token;
        mPreprocessor.lex(&token);
        EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
        EXPECT_EQ(str, token.text);
    }
};

class SingleLetterIdentifierTest : public IdentifierTest,
                                   public testing::WithParamInterface<char>
{
};


TEST_P(SingleLetterIdentifierTest, Identified)
{
    std::string str(1, GetParam());
    expectIdentifier(str);
}


INSTANTIATE_TEST_CASE_P(Underscore,
                        SingleLetterIdentifierTest,
                        testing::Values('_'));


INSTANTIATE_TEST_CASE_P(a_z,
                        SingleLetterIdentifierTest,
                        CLOSED_RANGE('a', 'z'));


INSTANTIATE_TEST_CASE_P(A_Z,
                        SingleLetterIdentifierTest,
                        CLOSED_RANGE('A', 'Z'));

typedef std::tr1::tuple<char, char> IdentifierParams;
class DoubleLetterIdentifierTest :
    public IdentifierTest,
    public testing::WithParamInterface<IdentifierParams>
{
};


TEST_P(DoubleLetterIdentifierTest, Identified)
{
    std::string str;
    str.push_back(std::tr1::get<0>(GetParam()));
    str.push_back(std::tr1::get<1>(GetParam()));

    expectIdentifier(str);
}


INSTANTIATE_TEST_CASE_P(Underscore_Underscore,
                        DoubleLetterIdentifierTest,
                        testing::Combine(testing::Values('_'),
                                         testing::Values('_')));


INSTANTIATE_TEST_CASE_P(Underscore_a_z,
                        DoubleLetterIdentifierTest,
                        testing::Combine(testing::Values('_'),
                                         CLOSED_RANGE('a', 'z')));


INSTANTIATE_TEST_CASE_P(Underscore_A_Z,
                        DoubleLetterIdentifierTest,
                        testing::Combine(testing::Values('_'),
                                         CLOSED_RANGE('A', 'Z')));


INSTANTIATE_TEST_CASE_P(Underscore_0_9,
                        DoubleLetterIdentifierTest,
                        testing::Combine(testing::Values('_'),
                                         CLOSED_RANGE('0', '9')));


INSTANTIATE_TEST_CASE_P(a_z_Underscore,
                        DoubleLetterIdentifierTest,
                        testing::Combine(CLOSED_RANGE('a', 'z'),
                                         testing::Values('_')));


INSTANTIATE_TEST_CASE_P(a_z_a_z,
                        DoubleLetterIdentifierTest,
                        testing::Combine(CLOSED_RANGE('a', 'z'),
                                         CLOSED_RANGE('a', 'z')));


INSTANTIATE_TEST_CASE_P(a_z_A_Z,
                        DoubleLetterIdentifierTest,
                        testing::Combine(CLOSED_RANGE('a', 'z'),
                                         CLOSED_RANGE('A', 'Z')));


INSTANTIATE_TEST_CASE_P(a_z_0_9,
                        DoubleLetterIdentifierTest,
                        testing::Combine(CLOSED_RANGE('a', 'z'),
                                         CLOSED_RANGE('0', '9')));


INSTANTIATE_TEST_CASE_P(A_Z_Underscore,
                        DoubleLetterIdentifierTest,
                        testing::Combine(CLOSED_RANGE('A', 'Z'),
                                         testing::Values('_')));


INSTANTIATE_TEST_CASE_P(A_Z_a_z,
                        DoubleLetterIdentifierTest,
                        testing::Combine(CLOSED_RANGE('A', 'Z'),
                                         CLOSED_RANGE('a', 'z')));


INSTANTIATE_TEST_CASE_P(A_Z_A_Z,
                        DoubleLetterIdentifierTest,
                        testing::Combine(CLOSED_RANGE('A', 'Z'),
                                         CLOSED_RANGE('A', 'Z')));


INSTANTIATE_TEST_CASE_P(A_Z_0_9,
                        DoubleLetterIdentifierTest,
                        testing::Combine(CLOSED_RANGE('A', 'Z'),
                                         CLOSED_RANGE('0', '9')));



TEST_F(IdentifierTest, AllLetters)
{
    std::string str;
    for (int c = 'a'; c <= 'z'; ++c)
        str.push_back(c);

    str.push_back('_');

    for (int c = 'A'; c <= 'Z'; ++c)
        str.push_back(c);

    str.push_back('_');

    for (int c = '0'; c <= '9'; ++c)
        str.push_back(c);

    expectIdentifier(str);
}
