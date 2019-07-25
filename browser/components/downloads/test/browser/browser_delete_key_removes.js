







function gen_test()
{
  const DownloadData = [
    { state: nsIDM.DOWNLOAD_FINISHED },
    { state: nsIDM.DOWNLOAD_FAILED },
    { state: nsIDM.DOWNLOAD_CANCELED },
    { state: nsIDM.DOWNLOAD_BLOCKED_PARENTAL },
    { state: nsIDM.DOWNLOAD_DIRTY },
    { state: nsIDM.DOWNLOAD_BLOCKED_POLICY },
  ];

  try {
    
    for (let yy in gen_resetState()) yield;

    
    for (let yy in gen_addDownloadRows(DownloadData)) yield;

    
    for (let yy in gen_openPanel()) yield;

    
    let richlistbox = document.getElementById("downloadsListBox");
    let statement = Services.downloads.DBConnection.createAsyncStatement(
                    "SELECT COUNT(*) FROM moz_downloads");
    try {
      
      statement.executeAsync({
        handleResult: function(aResultSet)
        {
          is(aResultSet.getNextRow().getResultByIndex(0),
             richlistbox.children.length,
             "The database and the number of downloads display matches");
        },
        handleError: function(aError)
        {
          Cu.reportError(aError);
        },
        handleCompletion: function(aReason)
        {
          testRunner.continueTest();
        }
      });
      yield;

      
      let len = DownloadData.length;
      for (let i = 0; i < len; i++) {
        
        waitForFocus(testRunner.continueTest);
        yield;

        
        EventUtils.synthesizeKey("VK_DELETE", {});

        
        statement.executeAsync({
          handleResult: function(aResultSet)
          {
            is(aResultSet.getNextRow().getResultByIndex(0),
               len - (i + 1),
               "The download was properly removed");
          },
          handleError: function(aError)
          {
            Cu.reportError(aError);
          },
          handleCompletion: function(aReason)
          {
            testRunner.continueTest();
          }
        });
        yield;
      }
    } finally {
      statement.finalize();
    }
  } finally {
    
    for (let yy in gen_resetState()) yield;
  }
}
