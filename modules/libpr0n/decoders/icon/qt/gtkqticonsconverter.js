




































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


function GtkQtIconsConverter() { };
GtkQtIconsConverter.prototype = {
  classID:          Components.ID("{c0783c34-a831-40c6-8c03-98c9f74cca45}"),
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIGtkQtIconsConverter]),
  convert: function(icon) { return this._gtk_qt_icons_table[icon]; },
  _gtk_qt_icons_table: {
    'about':
    0,
    'add':
    0,
    'apply':
    44, 
    'cancel':
    39, 
    'clear':
    45, 
    'color-picker':
    0,
    'copy':
    0,
    'close':
    43, 
    'cut':
    0,
    'delete':
    0,
    'dialog-error':
    0,
    'dialog-info':
    0,
    'dialog-question':
    12, 
    'dialog-warning':
    10, 
    'directory':
    37, 
    'file':
    24, 
    'find':
    0,
    'go-back-ltr':
    53, 
    'go-back-rtl':
    53, 
    'go-back':
    53, 
    'go-forward-ltr':
    54, 
    'go-forward-rtl':
    54, 
    'go-forward':
    54, 
    'go-up':
    49, 
    'goto-first':
    0,
    'goto-last':
    0,
    'help':
    7, 
    'home':
    55, 
    'info':
    9, 
    'jump-to':
    0,
    'media-pause':
    0,
    'media-play':
    0,
    'network':
    20, 
    'no':
    48, 
    'ok':
    38, 
    'open':
    21, 
    'orientation-landscape':
    0,
    'orientation-portrait':
    0,
    'paste':
    0,
    'preferences':
    34, 
    'print-preview':
    0,
    'print':
    0,
    'properties':
    0,
    'quit':
    0,
    'redo':
    0,
    'refresh':
    58, 
    'remove':
    0,
    'revert-to-saved':
    0,
    'save-as':
    42, 
    'save':
    42, 
    'select-all':
    0,
    'select-font':
    0,
    'stop':
    59, 
    'undelete':
    0,
    'undo':
    0,
    'yes':
    47, 
    'zoom-100':
    0,
    'zoom-in':
    0,
    'zoom-out':
    0
  },
}
var components = [GtkQtIconsConverter];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);

