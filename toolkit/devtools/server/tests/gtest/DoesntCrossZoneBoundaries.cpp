






#include "DevTools.h"

DEF_TEST(DoesntCrossZoneBoundaries, {
    
    JS::RootedObject newGlobal(cx, JS_NewGlobalObject(cx,
                                                      getGlobalClass(),
                                                      nullptr,
                                                      JS::FireOnNewGlobalHook));
    ASSERT_TRUE(newGlobal);
    JS::Zone* newZone = nullptr;
    {
      JSAutoCompartment ac(cx, newGlobal);
      ASSERT_TRUE(JS_InitStandardClasses(cx, newGlobal));
      newZone = js::GetContextZone(cx);
    }
    ASSERT_TRUE(newZone);
    ASSERT_NE(newZone, zone);

    
    
    JS::ZoneSet targetZones;
    ASSERT_TRUE(targetZones.init());
    ASSERT_TRUE(targetZones.put(zone));

    FakeNode nodeA(cx);
    FakeNode nodeB(cx);
    FakeNode nodeC(cx);

    nodeA.zone = zone;
    nodeB.zone = nullptr;
    nodeC.zone = newZone;

    AddEdge(nodeA, nodeB);
    AddEdge(nodeB, nodeC);

    ::testing::NiceMock<MockWriter> writer;

    
    ExpectWriteNode(writer, nodeA);

    
    
    ExpectWriteNode(writer, nodeB);

    

    JS::AutoCheckCannotGC noGC(rt);

    ASSERT_TRUE(WriteHeapGraph(cx,
                               JS::ubi::Node(&nodeA),
                               writer,
                                false,
                               &targetZones,
                               noGC));
  });
