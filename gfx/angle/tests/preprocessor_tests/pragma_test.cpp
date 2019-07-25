





#include "PreprocessorTest.h"
#include "Token.h"

class PragmaTest : public PreprocessorTest
{
};

TEST_F(PragmaTest, EmptyName)
{
    const char* str = "#pragma\n";
    const char* expected = "\n";

    using testing::_;
    
    EXPECT_CALL(mDirectiveHandler, handlePragma(_, _, _)).Times(0);
    
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str, expected);
}

TEST_F(PragmaTest, EmptyValue)
{
    const char* str = "#pragma foo\n";
    const char* expected = "\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handlePragma(pp::SourceLocation(0, 1), "foo", ""));
    
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str, expected);
}

TEST_F(PragmaTest, NameValue)
{
    const char* str = "#pragma foo(bar)\n";
    const char* expected = "\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handlePragma(pp::SourceLocation(0, 1), "foo", "bar"));
    
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str, expected);
}

TEST_F(PragmaTest, Comments)
{
    const char* str = "/*foo*/"
                      "#"
                      "/*foo*/"
                      "pragma"
                      "/*foo*/"
                      "foo"
                      "/*foo*/"
                      "("
                      "/*foo*/"
                      "bar"
                      "/*foo*/"
                      ")"
                      "/*foo*/"
                      "//foo"
                      "\n";
    const char* expected = "\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handlePragma(pp::SourceLocation(0, 1), "foo", "bar"));
    
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str, expected);
}

TEST_F(PragmaTest, MissingNewline)
{
    const char* str = "#pragma foo(bar)";
    const char* expected = "";

    using testing::_;
    
    EXPECT_CALL(mDirectiveHandler,
                handlePragma(pp::SourceLocation(0, 1), "foo", "bar"));
    
    EXPECT_CALL(mDiagnostics, print(pp::Diagnostics::EOF_IN_DIRECTIVE, _, _));

    preprocess(str, expected);
}

class InvalidPragmaTest : public PragmaTest,
                          public testing::WithParamInterface<const char*>
{
};

TEST_P(InvalidPragmaTest, Identified)
{
    const char* str = GetParam();
    const char* expected = "\n";

    using testing::_;
    
    EXPECT_CALL(mDirectiveHandler, handlePragma(_, _, _)).Times(0);
    
    EXPECT_CALL(mDiagnostics,
                print(pp::Diagnostics::UNRECOGNIZED_PRAGMA,
                      pp::SourceLocation(0, 1), _));

    preprocess(str, expected);
}

INSTANTIATE_TEST_CASE_P(All, InvalidPragmaTest, testing::Values(
    "#pragma 1\n",               
    "#pragma foo()\n",           
    "#pragma foo bar)\n",        
    "#pragma foo(bar\n",         
    "#pragma foo bar\n",         
    "#pragma foo(bar) baz\n"));  
