






























































const SORT_LOOKUP_TABLE = {
  title:        { key: "TITLE",        dir: "ASCENDING"  },
  tags:         { key: "TAGS",         dir: "ASCENDING"  },
  url:          { key: "URI",          dir: "ASCENDING"  },
  date:         { key: "DATE",         dir: "DESCENDING" },
  visitCount:   { key: "VISITCOUNT",   dir: "DESCENDING" },
  keyword:      { key: "KEYWORD",      dir: "ASCENDING"  },
  dateAdded:    { key: "DATEADDED",    dir: "DESCENDING" },
  lastModified: { key: "LASTMODIFIED", dir: "DESCENDING" },
  description:  { key:  "ANNOTATION",
                  dir:  "ASCENDING",
                  anno: "bookmarkProperties/description" }
};






const DEFAULT_SORT_KEY = "TITLE";



let prevSortDir = null;
let prevSortKey = null;














function checkSort(aTree, aSortingMode, aSortingAnno) {
  
  
  let res = aTree.result;
  isnot(res, null,
        "sanity check: placeContent.result should not return null");

  
  is(res.sortingMode, aSortingMode,
     "column should now have sortingMode " + aSortingMode);

  
  if ([Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_ASCENDING,
       Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_DESCENDING].
      indexOf(aSortingMode) >= 0) {
    is(res.sortingAnnotation, aSortingAnno,
       "column should now have sorting annotation " + aSortingAnno);
  }
}


















function setSort(aOrganizerWin, aTree, aUnsortFirst, aShouldFail, aCol, aDir) {
  if (aUnsortFirst) {
    aOrganizerWin.ViewMenu.setSortColumn();
    checkSort(aTree, Ci.nsINavHistoryQueryOptions.SORT_BY_NONE, "");

    
    prevSortKey = null;
    prevSortDir = null;
  }

  let failed = false;
  try {
    aOrganizerWin.ViewMenu.setSortColumn(aCol, aDir);

    
    if (!aCol && !aDir) {
      prevSortKey = null;
      prevSortDir = null;
    }
    else {
      if (aCol)
        prevSortKey = SORT_LOOKUP_TABLE[aCol.getAttribute("anonid")].key;
      else if (prevSortKey === null)
        prevSortKey = DEFAULT_SORT_KEY;

      if (aDir)
        prevSortDir = aDir.toUpperCase();
      else if (prevSortDir === null)
        prevSortDir = SORT_LOOKUP_TABLE[aCol.getAttribute("anonid")].dir;
    }
  } catch (exc) {
    failed = true;
  }

  is(failed, !!aShouldFail,
     "setSortColumn on column " +
     (aCol ? aCol.getAttribute("anonid") : "(no column)") +
     " with direction " + (aDir || "(no direction)") +
     " and table previously " + (aUnsortFirst ? "unsorted" : "sorted") +
     " should " + (aShouldFail ? "" : "not ") + "fail");
}









function testInvalid(aOrganizerWin, aPlaceContentTree) {
  
  let bogusCol = document.createElement("treecol");
  bogusCol.setAttribute("anonid", "bogusColumn");
  setSort(aOrganizerWin, aPlaceContentTree, true, true, bogusCol, "ascending");

  
  setSort(aOrganizerWin, aPlaceContentTree, false, false, null, "bogus dir");
  checkSort(aPlaceContentTree, Ci.nsINavHistoryQueryOptions.SORT_BY_NONE, "");
}












function testSortByColAndDir(aOrganizerWin, aPlaceContentTree, aUnsortFirst) {
  let cols = aPlaceContentTree.getElementsByTagName("treecol");
  ok(cols.length > 0, "sanity check: placeContent should contain columns");

  for (let i = 0; i < cols.length; i++) {
    let col = cols.item(i);
    ok(col.hasAttribute("anonid"),
       "sanity check: column " + col.id + " should have anonid");

    let colId = col.getAttribute("anonid");
    ok(colId in SORT_LOOKUP_TABLE,
       "sanity check: unexpected placeContent column anonid");

    let sortConst =
      "SORT_BY_" + SORT_LOOKUP_TABLE[colId].key + "_" +
      (aUnsortFirst ? SORT_LOOKUP_TABLE[colId].dir : prevSortDir);
    let expectedSortMode = Ci.nsINavHistoryQueryOptions[sortConst];
    let expectedAnno = SORT_LOOKUP_TABLE[colId].anno || "";

    
    setSort(aOrganizerWin, aPlaceContentTree, aUnsortFirst, false, col);
    checkSort(aPlaceContentTree, expectedSortMode, expectedAnno);

    
    ["ascending", "descending"].forEach(function (dir) {
      let sortConst =
        "SORT_BY_" + SORT_LOOKUP_TABLE[colId].key + "_" + dir.toUpperCase();
      let expectedSortMode = Ci.nsINavHistoryQueryOptions[sortConst];
      setSort(aOrganizerWin, aPlaceContentTree, aUnsortFirst, false, col, dir);
      checkSort(aPlaceContentTree, expectedSortMode, expectedAnno);
    });
  }
}











function testSortByDir(aOrganizerWin, aPlaceContentTree, aUnsortFirst) {
  ["ascending", "descending"].forEach(function (dir) {
    let key = (aUnsortFirst ? DEFAULT_SORT_KEY : prevSortKey);
    let sortConst = "SORT_BY_" + key + "_" + dir.toUpperCase();
    let expectedSortMode = Ci.nsINavHistoryQueryOptions[sortConst];
    setSort(aOrganizerWin, aPlaceContentTree, aUnsortFirst, false, null, dir);
    checkSort(aPlaceContentTree, expectedSortMode, "");
  });
}



function test() {
  waitForExplicitFinish();

  openLibrary(function (win) {
        let tree = win.document.getElementById("placeContent");
        isnot(tree, null, "sanity check: placeContent tree should exist");
        
        testSortByColAndDir(win, tree, true);
        testSortByColAndDir(win, tree, false);
        testSortByDir(win, tree, true);
        testSortByDir(win, tree, false);
        testInvalid(win, tree);
        
        setSort(win, tree, false, false);
        
        win.close();
        finish();
  });
}
