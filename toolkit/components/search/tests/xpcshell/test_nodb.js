













function run_test()
{
  removeMetadata();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "2");

  let search = Services.search; 

  do_test_pending();
  do_timeout(500,
             function()
             {
               
               
               
               let metadata = gProfD.clone();
               metadata.append("search-metadata.json");
               do_check_true(!metadata.exists());
               removeMetadata();

               do_test_finished();
             }
            );
}
