





#include "PreprocessorTest.h"
#include "Token.h"

class ErrorTest : public PreprocessorTest
{
};

TEST_F(ErrorTest, Empty)
{
    const char* str = "#error\n";
    const char* expected = "\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler, handleError(pp::SourceLocation(0, 1), ""));
    
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str, expected);
}

TEST_F(ErrorTest, OneTokenMessage)
{
    const char* str = "#error foo\n";
    const char* expected = "\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleError(pp::SourceLocation(0, 1), " foo"));
    
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str, expected);
}

TEST_F(ErrorTest, TwoTokenMessage)
{
    const char* str = "#error foo bar\n";
    const char* expected = "\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleError(pp::SourceLocation(0, 1), " foo bar"));
    
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str, expected);
}

TEST_F(ErrorTest, Comments)
{
    const char* str = "/*foo*/"
                      "#"
                      "/*foo*/"
                      "error"
                      "/*foo*/"
                      "foo"
                      "/*foo*/"
                      "bar"
                      "/*foo*/"
                      "//foo"
                      "\n";
    const char* expected = "\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleError(pp::SourceLocation(0, 1), " foo bar"));
    
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str, expected);
}

TEST_F(ErrorTest, MissingNewline)
{
    const char* str = "#error foo";
    const char* expected = "";

    using testing::_;
    
    EXPECT_CALL(mDirectiveHandler,
                handleError(pp::SourceLocation(0, 1), " foo"));
    
    EXPECT_CALL(mDiagnostics, print(pp::Diagnostics::EOF_IN_DIRECTIVE, _, _));

    preprocess(str, expected);
}
