





#include "gtest/gtest.h"
#include "nsGlobalWindow.h"

struct dialog_test {
  const char* input;
  const char* output;
};

void runTokenizeTest(dialog_test& test)
{
  NS_ConvertUTF8toUTF16 input(test.input);

  nsAString::const_iterator end;
  input.EndReading(end);

  nsAString::const_iterator iter;
  input.BeginReading(iter);

  nsAutoString result;
  nsAutoString token;

  while (nsGlobalWindow::TokenizeDialogOptions(token, iter, end)) {
    if (!result.IsEmpty()) {
      result.Append(',');
    }

    result.Append(token);
  }

  ASSERT_STREQ(test.output, NS_ConvertUTF16toUTF8(result).get()) << "Testing " << test.input;
}

void runTest(dialog_test& test)
{
  NS_ConvertUTF8toUTF16 input(test.input);

  nsAutoString result;
  nsGlobalWindow::ConvertDialogOptions(input, result);

  ASSERT_STREQ(test.output, NS_ConvertUTF16toUTF8(result).get()) << "Testing " << test.input;
}

TEST(GlobalWindowDialogOptions, TestDialogTokenize)
{
  dialog_test tests[] = {
    
    { "", "" },
    { " ", "" },
    { "   ", "" },

    
    { "a", "a" },
    { " a", "a" },
    { "  a  ", "a" },
    { "aa", "aa" },
    { " aa", "aa" },
    { "  aa  ", "aa" },
    { ";", ";" },
    { ":", ":" },
    { "=", "=" },

    
    { "a=", "a,=" },
    { "  a=  ", "a,=" },
    { "  a  =  ", "a,=" },
    { "aa=", "aa,=" },
    { "  aa=  ", "aa,=" },
    { "  aa  =  ", "aa,=" },
    { ";= ", ";,=" },
    { "==", "=,=" },
    { "::", ":,:" },

    
    { "a=2", "a,=,2" },
    { "===", "=,=,=" },
    { ";:=", ";,:,=" },

    
    { "aaa;bbb:ccc", "aaa,;,bbb,:,ccc" },

    
    { nullptr, nullptr }
  };

  for (uint32_t i = 0; tests[i].input; ++i) {
    runTokenizeTest(tests[i]);
  }
}
TEST(GlobalWindowDialogOptions, TestDialogOptions)
{
  dialog_test tests[] = {
    
    { "", "" },
    { " ", "" },
    { "   ", "" },

    
    { "a", "" },
    { " a", "" },
    { "  a  ", "" },
    { "a=", "" },
    { "  a=  ", "" },
    { "  a  =  ", "" },

    
    { "a=2", "" },
    { " a=2 ", "" },
    { "  a  =  2 ", "" },
    { "a:2", "" },
    { " a:2 ", "" },
    { "  a  :  2 ", "" },

    
    { "center=2", "" },
    { "center:2", "" },

    
    { "center=on", ",centerscreen=1" },
    { "center:on", ",centerscreen=1" },
    { " center : on ", ",centerscreen=1" },

    
    { " ; ", "" },

    
    { nullptr, nullptr }
  };

  for (uint32_t i = 0; tests[i].input; ++i) {
    runTest(tests[i]);
  }
}
