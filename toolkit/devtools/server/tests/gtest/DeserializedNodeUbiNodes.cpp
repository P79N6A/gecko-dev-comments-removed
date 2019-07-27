








#include "DevTools.h"
#include "js/TypeDecls.h"
#include "mozilla/devtools/DeserializedNode.h"

using testing::Field;
using testing::ReturnRef;


struct MockDeserializedNode : public DeserializedNode
{
  MockDeserializedNode(NodeId id, const char16_t* typeName, uint64_t size)
    : DeserializedNode(id, typeName, size)
  { }

  bool addEdge(DeserializedEdge&& edge)
  {
    return edges.append(Move(edge));
  }

  MOCK_METHOD1(getEdgeReferent, DeserializedNode&(const DeserializedEdge&));
};

size_t fakeMallocSizeOf(const void*) {
  EXPECT_TRUE(false);
  MOZ_ASSERT_UNREACHABLE("fakeMallocSizeOf should never be called because "
                         "DeserializedNodes report the deserialized size.");
  return 0;
}

DEF_TEST(DeserializedNodeUbiNodes, {
    const char16_t* typeName = MOZ_UTF16("TestTypeName");

    NodeId id = 1L << 33;
    uint64_t size = 1L << 60;
    MockDeserializedNode mocked(id, typeName, size);

    DeserializedNode& deserialized = mocked;
    JS::ubi::Node ubi(&deserialized);

    

    EXPECT_EQ(size, ubi.size(fakeMallocSizeOf));
    EXPECT_EQ(typeName, ubi.typeName());
    EXPECT_EQ(id, ubi.identifier());
    EXPECT_FALSE(ubi.isLive());

    

    UniquePtr<DeserializedNode> referent1(new MockDeserializedNode(1,
                                                                   nullptr,
                                                                   10));
    DeserializedEdge edge1;
    edge1.referent = referent1->id;
    mocked.addEdge(Move(edge1));
    EXPECT_CALL(mocked,
                getEdgeReferent(Field(&DeserializedEdge::referent,
                                      referent1->id)))
      .Times(1)
      .WillOnce(ReturnRef(*referent1.get()));

    UniquePtr<DeserializedNode> referent2(new MockDeserializedNode(2,
                                                                   nullptr,
                                                                   20));
    DeserializedEdge edge2;
    edge2.referent = referent2->id;
    mocked.addEdge(Move(edge2));
    EXPECT_CALL(mocked,
                getEdgeReferent(Field(&DeserializedEdge::referent,
                                      referent2->id)))
      .Times(1)
      .WillOnce(ReturnRef(*referent2.get()));

    UniquePtr<DeserializedNode> referent3(new MockDeserializedNode(3,
                                                                   nullptr,
                                                                   30));
    DeserializedEdge edge3;
    edge3.referent = referent3->id;
    mocked.addEdge(Move(edge3));
    EXPECT_CALL(mocked,
                getEdgeReferent(Field(&DeserializedEdge::referent,
                                      referent3->id)))
      .Times(1)
      .WillOnce(ReturnRef(*referent3.get()));

    ubi.edges(cx);
  });
