


















const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;


const IMAGE_FILTERS = ['image/gif', 'image/jpeg', 'image/pjpeg',
                       'image/png', 'image/svg+xml', 'image/tiff',
                       'image/vnd.microsoft.icon'];
const VIDEO_FILTERS = ['video/mpeg', 'video/mp4', 'video/ogg',
                       'video/quicktime', 'video/webm', 'video/x-matroska',
                       'video/x-ms-wmv', 'video/x-flv'];
const AUDIO_FILTERS = ['audio/basic', 'audio/L24', 'audio/mp4',
                       'audio/mpeg', 'audio/ogg', 'audio/vorbis',
                       'audio/vnd.rn-realaudio', 'audio/vnd.wave',
                       'audio/webm'];

Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyServiceGetter(this, 'cpmm',
                                   '@mozilla.org/childprocessmessagemanager;1',
                                   'nsIMessageSender');

function FilePicker() {
}

FilePicker.prototype = {
  classID: Components.ID('{436ff8f9-0acc-4b11-8ec7-e293efba3141}'),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFilePicker]),

  

  mParent: undefined,
  mFilterTypes: [],
  mFileEnumerator: undefined,
  mFilePickerShownCallback: undefined,

  

  init: function(parent, title, mode) {
    this.mParent = parent;

    if (mode != Ci.nsIFilePicker.modeOpen &&
        mode != Ci.nsIFilePicker.modeOpenMultiple) {
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    }
  },

  
  
  

  get domfiles() {
    return this.mFilesEnumerator;
  },

  get domfile() {
    return this.mFilesEnumerator ? this.mFilesEnumerator.mFiles[0] : null;
  },

  appendFilters: function(filterMask) {
    this.mFilterTypes = null;

    
    

    if (filterMask & Ci.nsIFilePicker.filterImages) {
      this.mFilterTypes = IMAGE_FILTERS;
    }

    
    
    
    

    if (filterMask & Ci.nsIFilePicker.filterVideo) {
      this.mFilterTypes = VIDEO_FILTERS;
    }

    if (filterMask & Ci.nsIFilePicker.filterAudio) {
      this.mFilterTypes = AUDIO_FILTERS;
    }

    
  },

  appendFilter: function(title, extensions) {
    
  },

  open: function(aFilePickerShownCallback) {
    this.mFilePickerShownCallback = aFilePickerShownCallback;

    cpmm.addMessageListener('file-picked', this);

    let detail = {};
    if (this.mFilterTypes) {
       detail.type = this.mFilterTypes;
    }

    cpmm.sendAsyncMessage('file-picker', detail);
  },

  fireSuccess: function(file) {
    this.mFilesEnumerator = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),

      mFiles: [file],
      mIndex: 0,

      hasMoreElements: function() {
        return (this.mIndex < this.mFiles.length);
      },

      getNext: function() {
        if (this.mIndex >= this.mFiles.length) {
          throw Components.results.NS_ERROR_FAILURE;
        }
        return this.mFiles[this.mIndex++];
      }
    };

    if (this.mFilePickerShownCallback) {
      this.mFilePickerShownCallback.done(Ci.nsIFilePicker.returnOK);
      this.mFilePickerShownCallback = null;
    }
  },

  fireError: function() {
    if (this.mFilePickerShownCallback) {
      this.mFilePickerShownCallback.done(Ci.nsIFilePicker.returnCancel);
      this.mFilePickerShownCallback = null;
    }
  },

  receiveMessage: function(message) {
    if (message.name !== 'file-picked') {
      return;
    }

    cpmm.removeMessageListener('file-picked', this);

    let data = message.data;
    if (!data.success || !data.result.blob) {
      this.fireError();
      return;
    }

    var mimeSvc = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService);
    var mimeInfo = mimeSvc.getFromTypeAndExtension(data.result.blob.type, '');

    var name = 'blob';
    if (mimeInfo) {
      name += '.' + mimeInfo.primaryExtension;
    }

    let file = new this.mParent.File(data.result.blob,
                                     { name: name,
                                       type: data.result.blob.type });
    if (file) {
      this.fireSuccess(file);
    } else {
      this.fireError();
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([FilePicker]);
