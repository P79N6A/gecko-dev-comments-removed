






#include "DevTools.h"

using testing::Field;
using testing::IsNull;
using testing::Property;
using testing::Return;

DEF_TEST(SerializesEdgeNames, {
    FakeNode node(cx);
    FakeNode referent(cx);

    const char16_t edgeName[] = MOZ_UTF16("edge name");
    const char16_t emptyStr[] = MOZ_UTF16("");

    AddEdge(node, referent, edgeName);
    AddEdge(node, referent, emptyStr);
    AddEdge(node, referent, nullptr);

    ::testing::NiceMock<MockWriter> writer;

    
    EXPECT_CALL(
      writer,
      writeNode(AllOf(EdgesLength(cx, 3),
                      Edge(cx, 0, Field(&JS::ubi::Edge::name,
                                        UTF16StrEq(edgeName))),
                      Edge(cx, 1, Field(&JS::ubi::Edge::name,
                                        UTF16StrEq(emptyStr))),
                      Edge(cx, 2, Field(&JS::ubi::Edge::name,
                                        IsNull()))),
                _)
    )
      .Times(1)
      .WillOnce(Return(true));

    
    ExpectWriteNode(writer, referent);

    JS::AutoCheckCannotGC noGC(rt);
    ASSERT_TRUE(WriteHeapGraph(cx,
                               JS::ubi::Node(&node),
                               writer,
                                true,
                                nullptr,
                               noGC));
  });
