








function gen_test()
{
  
  const DownloadData = [
    { endTime: 1180493839859239, state: nsIDM.DOWNLOAD_NOTSTARTED },
    { endTime: 1180493839859238, state: nsIDM.DOWNLOAD_DOWNLOADING },
    { endTime: 1180493839859237, state: nsIDM.DOWNLOAD_PAUSED },
    { endTime: 1180493839859236, state: nsIDM.DOWNLOAD_SCANNING },
    { endTime: 1180493839859235, state: nsIDM.DOWNLOAD_QUEUED },
    { endTime: 1180493839859234, state: nsIDM.DOWNLOAD_FINISHED },
    { endTime: 1180493839859233, state: nsIDM.DOWNLOAD_FAILED },
    { endTime: 1180493839859232, state: nsIDM.DOWNLOAD_CANCELED },
    { endTime: 1180493839859231, state: nsIDM.DOWNLOAD_BLOCKED_PARENTAL },
    { endTime: 1180493839859230, state: nsIDM.DOWNLOAD_DIRTY },
    { endTime: 1180493839859229, state: nsIDM.DOWNLOAD_BLOCKED_POLICY },
  ];

  try {
    
    for (let yy in gen_resetState()) yield;

    
    for (let yy in gen_addDownloadRows(DownloadData)) yield;

    
    for (let yy in gen_openPanel()) yield;

    
    let richlistbox = document.getElementById("downloadsListBox");




    for (let i = 0; i < richlistbox.children.length; i++) {
      let element = richlistbox.children[i];
      let dataItem = new DownloadsViewItemController(element).dataItem;
      is(dataItem.target, DownloadData[i].name, "Download names match up");
      is(dataItem.state, DownloadData[i].state, "Download states match up");
      is(dataItem.file, DownloadData[i].target, "Download targets match up");
      is(dataItem.uri, DownloadData[i].source, "Download sources match up");
    }
  } finally {
    
    for (let yy in gen_resetState()) yield;
  }
}
