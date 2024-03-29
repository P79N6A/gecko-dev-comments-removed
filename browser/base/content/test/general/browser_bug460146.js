

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  
  gBrowser.selectedBrowser.addEventListener("load", function () {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    var doc = gBrowser.contentDocument;
    var pageInfo = BrowserPageInfo(doc, "mediaTab");

    pageInfo.addEventListener("load", function () {
      pageInfo.removeEventListener("load", arguments.callee, true);
      pageInfo.onFinished.push(function () {
        executeSoon(function () {
          var imageTree = pageInfo.document.getElementById("imagetree");
          var imageRowsNum = imageTree.view.rowCount;

          ok(imageTree, "Image tree is null (media tab is broken)");

          ok(imageRowsNum == 7, "Number of images listed: " +
                                imageRowsNum + ", should be 7");

          pageInfo.close();
          gBrowser.removeCurrentTab();
          finish();
        });
      });
    }, true);
  }, true);

  content.location =
    "data:text/html," +
    "<html>" +
    "  <head>" +
    "    <title>Test for media tab</title>" +
    "    <link rel='shortcut icon' href='file:///dummy_icon.ico'>" + 
    "  </head>" +
    "  <body style='background-image:url(about:logo?a);'>" + 
    "    <img src='file:///dummy_image.gif'>" + 
    "    <ul>" +
    "      <li style='list-style:url(about:logo?b);'>List Item 1</li>" + 
    "    </ul>  " +
    "    <div style='-moz-border-image: url(about:logo?c) 20 20 20 20;'>test</div>" + 
    "    <a href='' style='cursor: url(about:logo?d),default;'>test link</a>" + 
    "    <object type='image/svg+xml' width=20 height=20 data='file:///dummy_object.svg'></object>" + 
    "  </body>" +
    "</html>";
}
