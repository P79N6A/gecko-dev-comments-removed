function test()
{
  

  var value = content.location.href;
  var urlString = value + "\n" + content.document.title;
  var htmlString = "<a href=\"" + value + "\">" + value + "</a>";

  var expected = [ [
    "text/x-moz-url: " + urlString,
    "text/uri-list: " + value,
    "text/plain: " + value,
    "text/html: " + htmlString
  ] ];

  
  var proxyicon = document.getElementById("page-proxy-favicon")
  var oldstate = proxyicon.getAttribute("pageproxystate");
  proxyicon.setAttribute("pageproxystate", "valid");
  var dt = EventUtils.synthesizeDragStart(proxyicon, expected);
  is(dt, null, "drag on proxy icon");
  proxyicon.setAttribute("pageproxystate", oldstate);
  
  
  EventUtils.synthesizeKey("VK_ESCAPE", {}, window);
}
