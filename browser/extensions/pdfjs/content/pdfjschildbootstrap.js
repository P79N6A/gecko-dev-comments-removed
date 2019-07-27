


















'use strict';






Components.utils.import('resource://gre/modules/Services.jsm');
Components.utils.import('resource://pdf.js/PdfJs.jsm');
Components.utils.import('resource://pdf.js/PdfjsContentUtils.jsm');


PdfjsContentUtils.init();

if (Services.appinfo.processType === Services.appinfo.PROCESS_TYPE_CONTENT) {
  
  PdfJs.updateRegistration();
}

