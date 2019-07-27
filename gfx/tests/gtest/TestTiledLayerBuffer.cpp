




#include "TiledLayerBuffer.h"

#include "gtest/gtest.h"

namespace mozilla {
namespace layers {

TEST(TiledLayerBuffer, TileStart) {
  gfxPlatform::GetPlatform()->ComputeTileSize();

  ASSERT_EQ(RoundDownToTileEdge(10, 256), 0);
  ASSERT_EQ(RoundDownToTileEdge(-10, 256), -256);
}

TEST(TiledLayerBuffer, TilesPlacement) {
  for (int firstY = -10; firstY < 10; ++firstY) {
    for (int firstX = -10; firstX < 10; ++firstX) {
      for (int height = 1; height < 10; ++height) {
        for (int width = 1; width < 10; ++width) {

          const TilesPlacement p1 = TilesPlacement(firstX, firstY, width, height);
          
          
          ASSERT_FALSE(p1.HasTile(TileIntPoint(firstX - 1, 0)));
          ASSERT_FALSE(p1.HasTile(TileIntPoint(0, firstY - 1)));
          ASSERT_FALSE(p1.HasTile(TileIntPoint(firstX + width + 1,  0)));
          ASSERT_FALSE(p1.HasTile(TileIntPoint(0, firstY + height + 1)));

          
          
          for (int y = firstY; y < (firstY+height); ++y) {
            for (int x = firstX; x < (firstX+width); ++x) {
              ASSERT_TRUE(p1.HasTile(TileIntPoint(x,y)));
              ASSERT_TRUE(p1.TileIndex(TileIntPoint(x, y)) >= 0);
              ASSERT_TRUE(p1.TileIndex(TileIntPoint(x, y)) < width * height);
            }
          }

          
          
          
          
          
          
          
          
          
          
          
          
          
          

        }
      }
    }
  }
}

}
}
