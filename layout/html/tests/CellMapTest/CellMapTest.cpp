



































#include <stdio.h>
#include "nscore.h"
#include "..\..\src\nsCellMap.h"

static FILE * out;

class CellData
{
public:
  PRInt32 mID;

  CellData(PRInt32 aID)
  { mID = aID;};

};

class BasicTest
{
public:
  BasicTest();
  virtual ~BasicTest() {};

  void DumpCellMap(nsCellMap *aMap, PRInt32 aRows, PRInt32 aCols);
  void VerifyRowsAndCols(nsCellMap *aMap, PRInt32 aRows, PRInt32 aCols);
};

BasicTest::BasicTest()
{
  PRInt32 rows = 3;
  PRInt32 cols = 4;
  nsCellMap *map = new nsCellMap(rows, cols);
  fprintf(out, "\nCreating %d by %d table with these values...\n", rows, cols);
  for (PRInt32 i=0; i<rows; i++)
  {
    for (PRInt32 j=0; j<cols; j++)
    {
      CellData * cellData = new CellData((i*cols)+j);
      fprintf(out, "cell_%d ", cellData->mID);
      map->SetCellAt(cellData, i, j);
    }
    fprintf(out, "\n");
  }
  
  DumpCellMap(map, rows, cols);
  VerifyRowsAndCols(map, rows, cols);

  
  map->DumpCellMap();

  
  fprintf(out, "\nadding a column, so cols=%d\n", cols);
  cols++;
  map->GrowTo(cols);
  DumpCellMap(map, rows, cols);
  VerifyRowsAndCols(map, rows, cols);

  
  rows++;  cols++;
  fprintf(out, "\nresetting the map to %d,%d\n", rows, cols);
  map->Reset(rows, cols);
  DumpCellMap(map, rows, cols);
  VerifyRowsAndCols(map, rows, cols);

}

void BasicTest::DumpCellMap(nsCellMap *aMap, PRInt32 aRows, PRInt32 aCols)
{
  fprintf(out, "\nPrinting out %d by %d table...\n", aRows, aCols);
  for (PRInt32 i=0; i<aRows; i++)
  {
    for (PRInt32 j=0; j<aCols; j++)
    {
      CellData * cellData = aMap->GetCellAt(i, j);
      if (nsnull!=cellData)
        fprintf(out, "Cell_%d ", cellData->mID);
      else
        fprintf(out, "null ");
    }
    fprintf(out, "\n");
  }
}

void BasicTest::VerifyRowsAndCols(nsCellMap *aMap, PRInt32 aRows, PRInt32 aCols)
{
  if (aMap->GetRowCount()!=aRows)
    fprintf(out, "ERROR:  map->mRowCount!=rows, %d != %d\n", aMap->GetRowCount(), aRows);
  if (aMap->GetColCount()!=aCols)
    fprintf(out, "ERROR:  map->mColCount!=cols, %d != %d\n", aMap->GetColCount(), aCols);
}

void main (int argc, char **argv)
{
  out = fopen("CellMapTest.txt", "w+t");
  if (nsnull==out)
  {
    printf("test failed to open output file\n");
    exit(-1);
  }
  fprintf(out, "Test starting...\n\n");
  BasicTest basicTest;
  fprintf(out, "\nTest completed.\n");
}



