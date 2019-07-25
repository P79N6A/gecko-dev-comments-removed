




function run_test()
{
  var domParser = Components.classes["@mozilla.org/xmlextras/domparser;1"]
                            .createInstance(Components.interfaces.nsIDOMParser);
  var aDom = domParser.parseFromString("<root><feed><entry/><entry/></feed></root>",
                                       "application/xml");
  var feedList = aDom.getElementsByTagName("feed");
  do_check_neq(feedList, null);
  do_check_eq(feedList.length, 1);
  do_check_neq(feedList[0], null);
  do_check_eq(feedList[0].tagName, "feed");
  var entry = feedList[0].getElementsByTagName("entry");
  do_check_neq(entry, null);
}
