function run_test()
{
  var dirEntries = do_get_file("xpcom/tests/unit/").directoryEntries;

  while (dirEntries.hasMoreElements())
    dirEntries.getNext();

  
  dirEntries.hasMoreElements();
}
