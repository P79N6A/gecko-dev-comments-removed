






































 



Components.utils.import("resource://gre/modules/utils.js");

var hs = PlacesUtils.history;
var bs = PlacesUtils.bookmarks;
var as = PlacesUtils.annotations;

const TEST_URL = "http://test.mozilla.org/";

function run_test() {
  var testURI = uri(TEST_URL);
  
  var itemId = bs.insertBookmark(bs.unfiledBookmarksFolder, testURI,
                                 bs.DEFAULT_INDEX, "test");

  
  var testAnnos = [{ name: "testAnno/test0",
                     type: Ci.nsIAnnotationService.TYPE_STRING,
                     flags: 0,
                     value: "test0",
                     expires: Ci.nsIAnnotationService.EXPIRE_NEVER },
                   { name: "testAnno/test1",
                     type: Ci.nsIAnnotationService.TYPE_STRING,
                     flags: 0,
                     value: "test1",
                     expires: Ci.nsIAnnotationService.EXPIRE_NEVER },
                   { name: "testAnno/test2",
                     type: Ci.nsIAnnotationService.TYPE_STRING,
                     flags: 0,
                     value: "test2",
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

  
  testAnnos.forEach(function(anno) { anno.value = null; });

  
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
