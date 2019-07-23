



































Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var mockFilePickerSettings = {
  




  destDir: null,

  



  filterIndex: -1
};

var mockFilePickerResults = {
  


  selectedFile: null,

  


  proposedFilterIndex: -1
};






function MockFilePicker() { };
MockFilePicker.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFilePicker]),
  init: function(aParent, aTitle, aMode) { },
  appendFilters: function(aFilterMask) { },
  appendFilter: function(aTitle, aFilter) { },
  defaultString: "",
  defaultExtension: "",
  filterIndex: 0,
  displayDirectory: null,
  file: null,
  get fileURL() {
    return Cc["@mozilla.org/network/io-service;1"].
           getService(Ci.nsIIOService).newFileURI(this.file);
  },
  get files() {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  show: function MFP_show() {
    
    
    
    this.file = mockFilePickerSettings.destDir.clone();
    this.file.append(this.defaultString || "no_default_file_name");
    
    mockFilePickerResults.selectedFile = this.file.clone();
    mockFilePickerResults.proposedFilterIndex = this.filterIndex;
    
    if (mockFilePickerSettings.filterIndex != -1)
      this.filterIndex = mockFilePickerSettings.filterIndex;
    
    return (this.file.exists() ?
            Ci.nsIFilePicker.returnReplace :
            Ci.nsIFilePicker.returnOK);
  }
};







var mockFilePickerRegisterer =
  new MockObjectRegisterer("@mozilla.org/filepicker;1",
                           MockFilePicker);
