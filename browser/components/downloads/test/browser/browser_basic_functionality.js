








function test_task()
{
  
  const DownloadData = [
    { state: nsIDM.DOWNLOAD_NOTSTARTED },
    { state: nsIDM.DOWNLOAD_PAUSED },
    { state: nsIDM.DOWNLOAD_FINISHED },
    { state: nsIDM.DOWNLOAD_FAILED },
    { state: nsIDM.DOWNLOAD_CANCELED },
  ];

  try {
    
    yield task_resetState();

    
    var originalCountLimit = DownloadsView.kItemCountLimit;
    DownloadsView.kItemCountLimit = DownloadData.length;
    registerCleanupFunction(function () {
      DownloadsView.kItemCountLimit = originalCountLimit;
    });

    
    yield task_addDownloads(DownloadData);

    
    yield task_openPanel();

    
    let richlistbox = document.getElementById("downloadsListBox");




    let itemCount = richlistbox.children.length;
    for (let i = 0; i < itemCount; i++) {
      let element = richlistbox.children[itemCount - i - 1];
      let dataItem = new DownloadsViewItemController(element).dataItem;
      is(dataItem.state, DownloadData[i].state, "Download states match up");
    }
  } finally {
    
    yield task_resetState();
  }
}
