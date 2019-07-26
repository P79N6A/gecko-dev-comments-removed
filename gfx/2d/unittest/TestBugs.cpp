




#include "TestBugs.h"
#include "2D.h"
#include <string.h>

using namespace mozilla;
using namespace mozilla::gfx;

TestBugs::TestBugs()
{
  REGISTER_TEST(TestBugs, CairoClip918671);
}

void
TestBugs::CairoClip918671()
{
  RefPtr<DrawTarget> dt = Factory::CreateDrawTarget(BACKEND_CAIRO,
                                                    IntSize(100, 100),
                                                    FORMAT_B8G8R8A8);
  RefPtr<DrawTarget> ref = Factory::CreateDrawTarget(BACKEND_CAIRO,
                                                     IntSize(100, 100),
                                                     FORMAT_B8G8R8A8);
  
  RefPtr<PathBuilder> pb1 = dt->CreatePathBuilder();
  pb1->MoveTo(Point(10, 10));
  pb1->LineTo(Point(90, 10));
  pb1->LineTo(Point(90, 20));
  pb1->LineTo(Point(10, 20));
  pb1->Close();
  pb1->MoveTo(Point(90, 90));
  pb1->LineTo(Point(91, 90));
  pb1->LineTo(Point(91, 91));
  pb1->LineTo(Point(91, 90));
  pb1->Close();

  RefPtr<Path> path1 = pb1->Finish();
  dt->PushClip(path1);

  
  RefPtr<PathBuilder> pb2 = dt->CreatePathBuilder();
  pb2->MoveTo(Point(50, 50));
  pb2->LineTo(Point(55, 51));
  pb2->LineTo(Point(54, 55));
  pb2->LineTo(Point(50, 56));
  pb2->Close();

  RefPtr<Path> path2 = pb2->Finish();
  dt->PushClip(path2);

  dt->FillRect(Rect(0, 0, 100, 100), ColorPattern(Color(1,0,0)));

  RefPtr<SourceSurface> surf1 = dt->Snapshot();
  RefPtr<SourceSurface> surf2 = ref->Snapshot();

  RefPtr<DataSourceSurface> dataSurf1 = surf1->GetDataSurface();
  RefPtr<DataSourceSurface> dataSurf2 = surf2->GetDataSurface();

  for (int y = 0; y < dt->GetSize().height; y++) {
    VERIFY(memcmp(dataSurf1->GetData() + y * dataSurf1->Stride(),
                  dataSurf2->GetData() + y * dataSurf2->Stride(),
                  dataSurf1->GetSize().width * 4) == 0);
  }

}

