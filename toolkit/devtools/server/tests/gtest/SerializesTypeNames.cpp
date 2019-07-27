






#include "DevTools.h"

using testing::Property;
using testing::Return;

DEF_TEST(SerializesTypeNames, {
    FakeNode node(cx);

    ::testing::NiceMock<MockWriter> writer;
    EXPECT_CALL(writer, writeNode(Property(&JS::ubi::Node::typeName,
                                           UTF16StrEq(MOZ_UTF16("FakeNode"))),
                                  _))
      .Times(1)
      .WillOnce(Return(true));

    JS::AutoCheckCannotGC noGC(rt);
    ASSERT_TRUE(WriteHeapGraph(cx,
                               JS::ubi::Node(&node),
                               writer,
                                true,
                                nullptr,
                               noGC));
  });
