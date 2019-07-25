







































let test_bookmarks = {
  menu: [
    { title: "Mozilla Firefox",
      children: [
        { title: "Help and Tutorials", 
          url: "http://en-us.www.mozilla.com/en-US/firefox/help/",
          icon: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABGdBTUEAAK/INwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAHWSURBVHjaYvz//z8DJQAggJiQOe/fv2fv7Oz8rays/N+VkfG/iYnJfyD/1+rVq7ffu3dPFpsBAAHEAHIBCJ85c8bN2Nj4vwsDw/8zQLwKiO8CcRoQu0DxqlWrdsHUwzBAAIGJmTNnPgYa9j8UqhFElwPxf2MIDeIrKSn9FwSJoRkAEEAM0DD4DzMAyPi/G+QKY4hh5WAXGf8PDQ0FGwJ22d27CjADAAIIrLmjo+MXA9R2kAHvGBA2wwx6B8W7od6CeQcggKCmCEL8bgwxYCbUIGTDVkHDBia+CuotgACCueD3TDQN75D4xmAvCoK9ARMHBzAw0AECiBHkAlC0Mdy7x9ABNA3obAZXIAa6iKEcGlMVQHwWyjYuL2d4v2cPg8vZswx7gHyAAAK7AOif7SAbOqCmn4Ha3AHFsIDtgPq/vLz8P4MSkJ2W9h8ggBjevXvHDo4FQUQg/kdypqCg4H8lUIACnQ/SOBMYI8bAsAJFPcj1AAEEjwVQqLpAbXmH5BJjqI0gi9DTAAgDBBCcAVLkgmQ7yKCZxpCQxqUZhAECCJ4XgMl493ug21ZD+aDAXH0WLM4A9MZPXJkJIIAwTAR5pQMalaCABQUULttBGCCAGCnNzgABBgAMJ5THwGvJLAAAAABJRU5ErkJggg=="
        },
        { title: "Customize Firefox",
          url: "http://en-us.www.mozilla.com/en-US/firefox/customize/",
          icon: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABGdBTUEAAK/INwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAHWSURBVHjaYvz//z8DJQAggJiQOe/fv2fv7Oz8rays/N+VkfG/iYnJfyD/1+rVq7ffu3dPFpsBAAHEAHIBCJ85c8bN2Nj4vwsDw/8zQLwKiO8CcRoQu0DxqlWrdsHUwzBAAIGJmTNnPgYa9j8UqhFElwPxf2MIDeIrKSn9FwSJoRkAEEAM0DD4DzMAyPi/G+QKY4hh5WAXGf8PDQ0FGwJ22d27CjADAAIIrLmjo+MXA9R2kAHvGBA2wwx6B8W7od6CeQcggKCmCEL8bgwxYCbUIGTDVkHDBia+CuotgACCueD3TDQN75D4xmAvCoK9ARMHBzAw0AECiBHkAlC0Mdy7x9ABNA3obAZXIAa6iKEcGlMVQHwWyjYuL2d4v2cPg8vZswx7gHyAAAK7AOif7SAbOqCmn4Ha3AHFsIDtgPq/vLz8P4MSkJ2W9h8ggBjevXvHDo4FQUQg/kdypqCg4H8lUIACnQ/SOBMYI8bAsAJFPcj1AAEEjwVQqLpAbXmH5BJjqI0gi9DTAAgDBBCcAVLkgmQ7yKCZxpCQxqUZhAECCJ4XgMl493ug21ZD+aDAXH0WLM4A9MZPXJkJIIAwTAR5pQMalaCABQUULttBGCCAGCnNzgABBgAMJ5THwGvJLAAAAABJRU5ErkJggg=="
        },
        { title: "Get Involved",
          url: "http://en-us.www.mozilla.com/en-US/firefox/community/",
          icon: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABGdBTUEAAK/INwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAHWSURBVHjaYvz//z8DJQAggJiQOe/fv2fv7Oz8rays/N+VkfG/iYnJfyD/1+rVq7ffu3dPFpsBAAHEAHIBCJ85c8bN2Nj4vwsDw/8zQLwKiO8CcRoQu0DxqlWrdsHUwzBAAIGJmTNnPgYa9j8UqhFElwPxf2MIDeIrKSn9FwSJoRkAEEAM0DD4DzMAyPi/G+QKY4hh5WAXGf8PDQ0FGwJ22d27CjADAAIIrLmjo+MXA9R2kAHvGBA2wwx6B8W7od6CeQcggKCmCEL8bgwxYCbUIGTDVkHDBia+CuotgACCueD3TDQN75D4xmAvCoK9ARMHBzAw0AECiBHkAlC0Mdy7x9ABNA3obAZXIAa6iKEcGlMVQHwWyjYuL2d4v2cPg8vZswx7gHyAAAK7AOif7SAbOqCmn4Ha3AHFsIDtgPq/vLz8P4MSkJ2W9h8ggBjevXvHDo4FQUQg/kdypqCg4H8lUIACnQ/SOBMYI8bAsAJFPcj1AAEEjwVQqLpAbXmH5BJjqI0gi9DTAAgDBBCcAVLkgmQ7yKCZxpCQxqUZhAECCJ4XgMl493ug21ZD+aDAXH0WLM4A9MZPXJkJIIAwTAR5pQMalaCABQUULttBGCCAGCnNzgABBgAMJ5THwGvJLAAAAABJRU5ErkJggg=="
        },
        { title: "About Us",
          url: "http://en-us.www.mozilla.com/en-US/about/",
          icon: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABGdBTUEAAK/INwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAHWSURBVHjaYvz//z8DJQAggJiQOe/fv2fv7Oz8rays/N+VkfG/iYnJfyD/1+rVq7ffu3dPFpsBAAHEAHIBCJ85c8bN2Nj4vwsDw/8zQLwKiO8CcRoQu0DxqlWrdsHUwzBAAIGJmTNnPgYa9j8UqhFElwPxf2MIDeIrKSn9FwSJoRkAEEAM0DD4DzMAyPi/G+QKY4hh5WAXGf8PDQ0FGwJ22d27CjADAAIIrLmjo+MXA9R2kAHvGBA2wwx6B8W7od6CeQcggKCmCEL8bgwxYCbUIGTDVkHDBia+CuotgACCueD3TDQN75D4xmAvCoK9ARMHBzAw0AECiBHkAlC0Mdy7x9ABNA3obAZXIAa6iKEcGlMVQHwWyjYuL2d4v2cPg8vZswx7gHyAAAK7AOif7SAbOqCmn4Ha3AHFsIDtgPq/vLz8P4MSkJ2W9h8ggBjevXvHDo4FQUQg/kdypqCg4H8lUIACnQ/SOBMYI8bAsAJFPcj1AAEEjwVQqLpAbXmH5BJjqI0gi9DTAAgDBBCcAVLkgmQ7yKCZxpCQxqUZhAECCJ4XgMl493ug21ZD+aDAXH0WLM4A9MZPXJkJIIAwTAR5pQMalaCABQUULttBGCCAGCnNzgABBgAMJ5THwGvJLAAAAABJRU5ErkJggg=="
        },
      ],
    },
    { title: "test",
      description: "folder test comment",
      dateAdded: 1177541020000000,
      lastModified: 1177541050000000,
      children: [
        { title: "test post keyword",
          description: "item description",
          dateAdded: 1177375336000000,
          lastModified: 1177375423000000,
          keyword: "test",
          sidebar: true,
          postData: "hidden1%3Dbar&text1%3D%25s",
          charset: "ISO-8859-1",
        },
      ]
    },
  ],
  toolbar: [
    { title: "Getting Started",
      url: "http://en-us.www.mozilla.com/en-US/firefox/central/",
      icon: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABGdBTUEAAK/INwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAHWSURBVHjaYvz//z8DJQAggJiQOe/fv2fv7Oz8rays/N+VkfG/iYnJfyD/1+rVq7ffu3dPFpsBAAHEAHIBCJ85c8bN2Nj4vwsDw/8zQLwKiO8CcRoQu0DxqlWrdsHUwzBAAIGJmTNnPgYa9j8UqhFElwPxf2MIDeIrKSn9FwSJoRkAEEAM0DD4DzMAyPi/G+QKY4hh5WAXGf8PDQ0FGwJ22d27CjADAAIIrLmjo+MXA9R2kAHvGBA2wwx6B8W7od6CeQcggKCmCEL8bgwxYCbUIGTDVkHDBia+CuotgACCueD3TDQN75D4xmAvCoK9ARMHBzAw0AECiBHkAlC0Mdy7x9ABNA3obAZXIAa6iKEcGlMVQHwWyjYuL2d4v2cPg8vZswx7gHyAAAK7AOif7SAbOqCmn4Ha3AHFsIDtgPq/vLz8P4MSkJ2W9h8ggBjevXvHDo4FQUQg/kdypqCg4H8lUIACnQ/SOBMYI8bAsAJFPcj1AAEEjwVQqLpAbXmH5BJjqI0gi9DTAAgDBBCcAVLkgmQ7yKCZxpCQxqUZhAECCJ4XgMl493ug21ZD+aDAXH0WLM4A9MZPXJkJIIAwTAR5pQMalaCABQUULttBGCCAGCnNzgABBgAMJ5THwGvJLAAAAABJRU5ErkJggg=="
    },
    { title: "Latest Headlines",
      description: "Livemark test comment",
      url: "http://en-us.fxfeeds.mozilla.com/en-US/firefox/livebookmarks/",
      feedUrl: "http://en-us.fxfeeds.mozilla.com/en-US/firefox/headlines.xml",
    }
  ],
  unfiled: [
    { title: "Example.tld",
      url: "http://example.tld/",
    },
  ],
};


let gBookmarksFileOld;

let gBookmarksFileNew;

let importer = Cc["@mozilla.org/browser/places/import-export-service;1"].
               getService(Ci.nsIPlacesImportExportService);

function run_test()
{
  
  Services.prefs.setIntPref("browser.places.smartBookmarksVersion", -1);

  
  gBookmarksFileOld = do_get_file("bookmarks.preplaces.html");

  
  gBookmarksFileNew = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
  gBookmarksFileNew.append("bookmarks.exported.html");
  if (gBookmarksFileNew.exists()) {
    gBookmarksFileNew.remove(false);
  }
  gBookmarksFileNew.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
  if (!gBookmarksFileNew.exists()) {
    do_throw("couldn't create file: bookmarks.exported.html");
  }

  
  
  
  
  
  try {
    importer.importHTMLFromFile(gBookmarksFileOld, true);
  } catch(ex) { do_throw("couldn't import legacy bookmarks file: " + ex); }

  testImportedBookmarks();

  
  try {
    importer.exportHTMLToFile(gBookmarksFileNew);
  } catch(ex) { do_throw("couldn't export to file: " + ex); }

  remove_all_bookmarks();
  run_next_test();
}

add_test(function test_import_new()
{
  
  
  

  try {
    importer.importHTMLFromFile(gBookmarksFileNew, true);
  } catch(ex) { do_throw("couldn't import the exported file: " + ex); }

  testImportedBookmarks();

  remove_all_bookmarks();
  run_next_test();
});

add_test(function test_emptytitle_export()
{
  
  
  
  
  
  
  

  try {
    importer.importHTMLFromFile(gBookmarksFileNew, true);
  } catch(ex) { do_throw("couldn't import the exported file: " + ex); }

  const NOTITLE_URL = "http://notitle.mozilla.org/";
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                NetUtil.newURI(NOTITLE_URL),
                                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                "");
  test_bookmarks.unfiled.push({ title: "", url: NOTITLE_URL });

  try {
    importer.exportHTMLToFile(gBookmarksFileNew);
  } catch(ex) { do_throw("couldn't export to file: " + ex); }

  remove_all_bookmarks();

  try {
    importer.importHTMLFromFile(gBookmarksFileNew, true);
  } catch(ex) { do_throw("couldn't import the exported file: " + ex); }

  testImportedBookmarks();

  
  test_bookmarks.unfiled.pop();
  PlacesUtils.bookmarks.removeItem(id);

  try {
    importer.exportHTMLToFile(gBookmarksFileNew);
  } catch(ex) { do_throw("couldn't export to file: " + ex); }

  remove_all_bookmarks();
  run_next_test();
});

add_test(function test_import_preplaces_to_folder()
{
  
  
  
  

  let testFolder = PlacesUtils.bookmarks.createFolder(
    PlacesUtils.bookmarksMenuFolderId, "test-import",
    PlacesUtils.bookmarks.DEFAULT_INDEX
  );
  try {
    importer.importHTMLFromFileToFolder(gBookmarksFileOld, testFolder, false);
  } catch(ex) { do_throw("couldn't import the exported file to folder: " + ex); }

  
  testImportedBookmarksToFolder(testFolder);

  remove_all_bookmarks();
  run_next_test();
});

add_test(function test_import_to_folder()
{
  
  
  
  

  let testFolder = PlacesUtils.bookmarks.createFolder(
    PlacesUtils.bookmarksMenuFolderId, "test-import",
    PlacesUtils.bookmarks.DEFAULT_INDEX
  );
  try {
    importer.importHTMLFromFileToFolder(gBookmarksFileNew, testFolder, false);
  } catch(ex) { do_throw("couldn't import the exported file to folder: " + ex); }

  
  testImportedBookmarksToFolder(testFolder);

  remove_all_bookmarks();
  run_next_test();
});

add_test(function test_import_ontop()
{
  
  
  
  
  
  
  

  try {
    importer.importHTMLFromFile(gBookmarksFileNew, true);
  } catch(ex) { do_throw("couldn't import the exported file: " + ex); }
  try {
    importer.exportHTMLToFile(gBookmarksFileNew);
  } catch(ex) { do_throw("couldn't export to file: " + ex); }
  try {
    importer.importHTMLFromFile(gBookmarksFileNew, true);
  } catch(ex) { do_throw("couldn't import the exported file: " + ex); }

  testImportedBookmarks();

  remove_all_bookmarks();
  run_next_test();
});

function testImportedBookmarks()
{
  for (let group in test_bookmarks) {
    let root;
    switch (group) {
      case "menu":
        root = PlacesUtils.getFolderContents(PlacesUtils.bookmarksMenuFolderId).root;
        break;
      case "toolbar":
        root = PlacesUtils.getFolderContents(PlacesUtils.toolbarFolderId).root;
        break;
      case "unfiled":
        root = PlacesUtils.getFolderContents(PlacesUtils.unfiledBookmarksFolderId).root;
        break;
    }

    let items = test_bookmarks[group];
    do_check_eq(root.childCount, items.length);

    items.forEach(function (item, index) checkItem(item, root.getChild(index)));

    root.containerOpen = false;
  }
}

function testImportedBookmarksToFolder(aFolder)
{
  root = PlacesUtils.getFolderContents(aFolder).root;

  
  
  let rootFolderCount = test_bookmarks.menu.length;

  for (let i = 0; i < root.childCount; i++) {
    let child = root.getChild(i);
    if (i < rootFolderCount) {
      checkItem(test_bookmarks.menu[i], child);
    }
    else {
      let container = child.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      let group = /Toolbar/.test(container.title) ? test_bookmarks.toolbar
                                                  : test_bookmarks.unfiled;
      container.containerOpen = true;
      print(container.title);
      for (let t = 0; t < container.childCount; t++) {
        print(group[t].title + " " + container.getChild(t).title);
        checkItem(group[t], container.getChild(t));
      }
      container.containerOpen = false;
    }
  }

  root.containerOpen = false;
}

function checkItem(aExpected, aNode)
{
  let id = aNode.itemId;
  for (prop in aExpected) {
    switch (prop) {
      case "title":
        do_check_eq(aNode.title, aExpected.title);
        break;
      case "description":
        do_check_eq(PlacesUtils.annotations
                               .getItemAnnotation(id, PlacesUIUtils.DESCRIPTION_ANNO),
                    aExpected.description);
        break;
      case "dateAdded":
          do_check_eq(PlacesUtils.bookmarks.getItemDateAdded(id),
                      aExpected.dateAdded);
        break;
      case "lastModified":
          do_check_eq(PlacesUtils.bookmarks.getItemLastModified(id),
                      aExpected.lastModified);
        break;
      case "url":
        if (!PlacesUtils.livemarks.isLivemark(id))
          do_check_eq(aNode.uri, aExpected.url);
        break;
      case "icon":
        let faviconURI = PlacesUtils.favicons.getFaviconForPage(
          NetUtil.newURI(aExpected.url)
        );
        let dataURL = PlacesUtils.favicons.getFaviconDataAsDataURL(faviconURI);
        
        do_check_true(dataURL == aExpected.icon);
        break;
      case "keyword":
        break;
      case "sidebar":
        do_check_eq(PlacesUtils.annotations
                               .itemHasAnnotation(id, PlacesUIUtils.LOAD_IN_SIDEBAR_ANNO),
                    aExpected.sidebar);
        break;
      case "postData":
        do_check_eq(PlacesUtils.annotations
                               .getItemAnnotation(id, PlacesUtils.POST_DATA_ANNO),
                    aExpected.postData);
        break;
      case "charset":
        do_check_eq(PlacesUtils.history.getCharsetForURI(NetUtil.newURI(aNode.uri)),
                    aExpected.charset);
        break;
      case "feedUrl":
        do_check_true(PlacesUtils.livemarks.isLivemark(id));
        do_check_eq(PlacesUtils.livemarks.getSiteURI(id).spec,
                    aExpected.url);
        do_check_eq(PlacesUtils.livemarks.getFeedURI(id).spec,
                    aExpected.feedUrl);
        break;
      case "children":
        let folder = aNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
        do_check_eq(folder.hasChildren, aExpected.children.length > 0);
        folder.containerOpen = true;
        do_check_eq(folder.childCount, aExpected.children.length);

        aExpected.children.forEach(function (item, index) checkItem(item, folder.getChild(index)));

        folder.containerOpen = false;
        break;
      default:
        throw new Error("Unknown property");
    }
  };
}
