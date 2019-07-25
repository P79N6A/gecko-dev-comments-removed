




function run_test()
{
  do_test_pending();

  let annoObserver = {
    onItemAnnotationSet:
    function AO_onItemAnnotationSet(aItemId, aAnnotationName)
    {
      if (aAnnotationName == PlacesUtils.LMANNO_FEEDURI) {
        PlacesUtils.annotations.removeObserver(this);
        do_check_true(PlacesUtils.livemarks.isLivemark(aItemId));
        do_execute_soon(function () {
          PlacesUtils.bookmarks.removeItem(aItemId);
          do_check_false(PlacesUtils.livemarks.isLivemark(aItemId));
          do_test_finished();
        });
      }
    },
  
    onItemAnnotationRemoved: function () {},
    onPageAnnotationSet: function() {},
    onPageAnnotationRemoved: function() {},
    QueryInterface: XPCOMUtils.generateQI([
      Ci.nsIAnnotationObserver
    ]),
  }
  PlacesUtils.annotations.addObserver(annoObserver, false);
  PlacesUtils.livemarks.createLivemarkFolderOnly(
    PlacesUtils.unfiledBookmarksFolderId,
    "livemark title",
    uri("http://example.com/"),
    uri("http://example.com/rdf"),
    PlacesUtils.bookmarks.DEFAULT_INDEX
  );
}
