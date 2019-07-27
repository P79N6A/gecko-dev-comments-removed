




registerCleanupFunction(function*() {
  yield task_resetState();
});





add_task(function* test_basic_functionality() {
  
  const DownloadData = [
    { state: nsIDM.DOWNLOAD_NOTSTARTED },
    { state: nsIDM.DOWNLOAD_PAUSED },
    { state: nsIDM.DOWNLOAD_FINISHED },
    { state: nsIDM.DOWNLOAD_FAILED },
    { state: nsIDM.DOWNLOAD_CANCELED },
  ];

  
  yield promiseFocus();

  
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
    let download = DownloadsView.controllerForElement(element).download;
    is(DownloadsCommon.stateOfDownload(download), DownloadData[i].state,
       "Download states match up");
  }
});
