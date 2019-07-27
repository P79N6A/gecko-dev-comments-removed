






#include "DevTools.h"

DEF_TEST(SerializesEverythingInHeapGraphOnce, {
    FakeNode nodeA(cx);
    FakeNode nodeB(cx);
    FakeNode nodeC(cx);
    FakeNode nodeD(cx);

    AddEdge(nodeA, nodeB);
    AddEdge(nodeB, nodeC);
    AddEdge(nodeC, nodeD);
    AddEdge(nodeD, nodeA);

    ::testing::NiceMock<MockWriter> writer;

    
    ExpectWriteNode(writer, nodeA);
    ExpectWriteNode(writer, nodeB);
    ExpectWriteNode(writer, nodeC);
    ExpectWriteNode(writer, nodeD);

    JS::AutoCheckCannotGC noGC(rt);

    ASSERT_TRUE(WriteHeapGraph(cx,
                               JS::ubi::Node(&nodeA),
                               writer,
                                false,
                                nullptr,
                               noGC));
  });
