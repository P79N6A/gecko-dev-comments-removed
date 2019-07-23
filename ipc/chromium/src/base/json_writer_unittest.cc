



#include "testing/gtest/include/gtest/gtest.h"
#include "base/json_writer.h"
#include "base/values.h"

TEST(JSONWriterTest, Writing) {
  
  Value* root = Value::CreateNullValue();
  std::string output_js;
  JSONWriter::Write(root, false, &output_js);
  ASSERT_EQ("null", output_js);
  delete root;

  
  root = new DictionaryValue;
  JSONWriter::Write(root, false, &output_js);
  ASSERT_EQ("{}", output_js);
  delete root;

  
  root = new ListValue;
  JSONWriter::Write(root, false, &output_js);
  ASSERT_EQ("[]", output_js);
  delete root;

  
  root = Value::CreateRealValue(1.0);
  JSONWriter::Write(root, false, &output_js);
  ASSERT_EQ("1.0", output_js);
  delete root;

  
  root = Value::CreateRealValue(0.2);
  JSONWriter::Write(root, false, &output_js);
  ASSERT_EQ("0.2", output_js);
  delete root;

  
  root = Value::CreateRealValue(-0.8);
  JSONWriter::Write(root, false, &output_js);
  ASSERT_EQ("-0.8", output_js);
  delete root;
  
  
  
  DictionaryValue root_dict;
  ListValue* list = new ListValue;
  root_dict.Set(L"list", list);
  DictionaryValue* inner_dict = new DictionaryValue;
  list->Append(inner_dict);
  inner_dict->SetInteger(L"inner int", 10);
  ListValue* inner_list = new ListValue;
  list->Append(inner_list);
  list->Append(Value::CreateBooleanValue(true));

  JSONWriter::Write(&root_dict, false, &output_js);
  ASSERT_EQ("{\"list\":[{\"inner int\":10},[],true]}", output_js);
  JSONWriter::Write(&root_dict, true, &output_js);
  ASSERT_EQ("{\r\n"
            "   \"list\": [ {\r\n"
            "      \"inner int\": 10\r\n"
            "   }, [  ], true ]\r\n"
            "}\r\n",
            output_js);
}
