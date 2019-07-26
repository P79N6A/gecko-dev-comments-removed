





const charset = "UTF-8";
const CHARSET_ANNO = "URIProperties/characterSet";

const TEST_URI = uri("http://foo.com");
const TEST_BOOKMARKED_URI = uri("http://bar.com");

function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  
  yield promiseAddVisits(TEST_URI);
  yield promiseAddVisits(TEST_BOOKMARKED_URI);

  
  var bm1 = PlacesUtils.bookmarks.insertBookmark(
              PlacesUtils.unfiledBookmarksFolderId,
              TEST_BOOKMARKED_URI, PlacesUtils.bookmarks.DEFAULT_INDEX,
              TEST_BOOKMARKED_URI.spec);
  var bm2 = PlacesUtils.bookmarks.insertBookmark(
              PlacesUtils.toolbarFolderId,
              TEST_BOOKMARKED_URI, PlacesUtils.bookmarks.DEFAULT_INDEX,
              TEST_BOOKMARKED_URI.spec);

  
  yield PlacesUtils.setCharsetForURI(TEST_URI, charset);
  
  yield PlacesUtils.setCharsetForURI(TEST_BOOKMARKED_URI, charset);

  
  do_check_eq(PlacesUtils.annotations.getPageAnnotation(TEST_URI, CHARSET_ANNO), charset);

  
  do_check_eq((yield PlacesUtils.getCharsetForURI(TEST_URI)), charset);

  
  do_check_eq((yield PlacesUtils.getCharsetForURI(TEST_BOOKMARKED_URI)), charset);

  yield promiseClearHistory();

  
  do_check_neq((yield PlacesUtils.getCharsetForURI(TEST_URI)), charset);

  
  try {
    PlacesUtils.annotations.getPageAnnotation(TEST_URI, CHARSET_ANNO);
    do_throw("Charset page annotation has not been removed correctly");
  } catch (e) {}

  
  do_check_eq((yield PlacesUtils.getCharsetForURI(TEST_BOOKMARKED_URI)), charset);

  
  yield PlacesUtils.setCharsetForURI(TEST_BOOKMARKED_URI, "");
  do_check_neq((yield PlacesUtils.getCharsetForURI(TEST_BOOKMARKED_URI)), charset);
});
