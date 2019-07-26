





 



var hs = PlacesUtils.history;
var bs = PlacesUtils.bookmarks;
var as = PlacesUtils.annotations;

const TEST_URL = "http://test.mozilla.org/";

function run_test() {
  var testURI = uri(TEST_URL);
  
  var itemId = bs.insertBookmark(bs.unfiledBookmarksFolder, testURI,
                                 bs.DEFAULT_INDEX, "test");

  
  var testAnnos = [{ name: "testAnno/test0",
                     flags: 0,
                     value: "test0",
                     expires: Ci.nsIAnnotationService.EXPIRE_NEVER },
                   { name: "testAnno/test1",
                     flags: 0,
                     value: "test1",
                     expires: Ci.nsIAnnotationService.EXPIRE_NEVER },
                   { name: "testAnno/test2",
                     flags: 0,
                     value: "test2",
                     expires: Ci.nsIAnnotationService.EXPIRE_NEVER },
                   { name: "testAnno/test3",
                     flags: 0,
                     value: 0,
                     expires: Ci.nsIAnnotationService.EXPIRE_NEVER }];

  
  PlacesUtils.setAnnotationsForItem(itemId, testAnnos);
  
  testAnnos.forEach(function(anno) {
    do_check_true(as.itemHasAnnotation(itemId, anno.name));
    do_check_eq(as.getItemAnnotation(itemId, anno.name), anno.value);
  });

  
  PlacesUtils.setAnnotationsForURI(testURI, testAnnos);
  
  testAnnos.forEach(function(anno) {
    do_check_true(as.pageHasAnnotation(testURI, anno.name));
    do_check_eq(as.getPageAnnotation(testURI, anno.name), anno.value);
  });

  
  
  testAnnos[0].value = null;
  testAnnos[1].value = undefined;
  delete testAnnos[2].value;
  delete testAnnos[3].value;

  
  PlacesUtils.setAnnotationsForItem(itemId, testAnnos);
  
  testAnnos.forEach(function(anno) {
    do_check_false(as.itemHasAnnotation(itemId, anno.name));
    
    do_check_true(as.pageHasAnnotation(testURI, anno.name));
  });

  
  PlacesUtils.setAnnotationsForURI(testURI, testAnnos);
  
  testAnnos.forEach(function(anno) {
    do_check_false(as.pageHasAnnotation(testURI, anno.name));
  });
}
