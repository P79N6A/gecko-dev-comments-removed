

















'use strict';

var DEFAULT_URL = 'compressed.tracemonkey-pldi-09.pdf';
var DEFAULT_SCALE = 'auto';
var DEFAULT_SCALE_DELTA = 1.1;
var UNKNOWN_SCALE = 0;
var CACHE_SIZE = 20;
var CSS_UNITS = 96.0 / 72.0;
var SCROLLBAR_PADDING = 40;
var VERTICAL_PADDING = 5;
var MIN_SCALE = 0.25;
var MAX_SCALE = 4.0;
var IMAGE_DIR = './images/';
var SETTINGS_MEMORY = 20;
var ANNOT_MIN_SIZE = 10;
var RenderingStates = {
  INITIAL: 0,
  RUNNING: 1,
  PAUSED: 2,
  FINISHED: 3
};
var FindStates = {
  FIND_FOUND: 0,
  FIND_NOTFOUND: 1,
  FIND_WRAPPED: 2,
  FIND_PENDING: 3
};

  PDFJS.workerSrc = '../build/pdf.js';

var mozL10n = document.mozL10n || document.webL10n;

function getFileName(url) {
  var anchor = url.indexOf('#');
  var query = url.indexOf('?');
  var end = Math.min(
    anchor > 0 ? anchor : url.length,
    query > 0 ? query : url.length);
  return url.substring(url.lastIndexOf('/', end) + 1, end);
}

function scrollIntoView(element, spot) {
  
  
  
  var parent = element.offsetParent;
  var offsetY = element.offsetTop + element.clientTop;
  if (!parent) {
    console.error('offsetParent is not set -- cannot scroll');
    return;
  }
  while (parent.clientHeight == parent.scrollHeight) {
    offsetY += parent.offsetTop;
    parent = parent.offsetParent;
    if (!parent)
      return; 
  }
  if (spot)
    offsetY += spot.top;
  parent.scrollTop = offsetY;
}

var Cache = function cacheCache(size) {
  var data = [];
  this.push = function cachePush(view) {
    var i = data.indexOf(view);
    if (i >= 0)
      data.splice(i);
    data.push(view);
    if (data.length > size)
      data.shift().destroy();
  };
};

var ProgressBar = (function ProgressBarClosure() {

  function clamp(v, min, max) {
    return Math.min(Math.max(v, min), max);
  }

  function ProgressBar(id, opts) {

    
    this.div = document.querySelector(id + ' .progress');

    
    this.height = opts.height || 100;
    this.width = opts.width || 100;
    this.units = opts.units || '%';

    
    this.div.style.height = this.height + this.units;
  }

  ProgressBar.prototype = {

    updateBar: function ProgressBar_updateBar() {
      if (this._indeterminate) {
        this.div.classList.add('indeterminate');
        return;
      }

      var progressSize = this.width * this._percent / 100;

      if (this._percent > 95)
        this.div.classList.add('full');
      else
        this.div.classList.remove('full');
      this.div.classList.remove('indeterminate');

      this.div.style.width = progressSize + this.units;
    },

    get percent() {
      return this._percent;
    },

    set percent(val) {
      this._indeterminate = isNaN(val);
      this._percent = clamp(val, 0, 100);
      this.updateBar();
    }
  };

  return ProgressBar;
})();
















var FirefoxCom = (function FirefoxComClosure() {
  'use strict';

  return {
    








    requestSync: function(action, data) {
      var request = document.createTextNode('');
      document.documentElement.appendChild(request);

      var sender = document.createEvent('CustomEvent');
      sender.initCustomEvent('pdf.js.message', true, false,
                             {action: action, data: data, sync: true});
      request.dispatchEvent(sender);
      var response = sender.detail.response;
      document.documentElement.removeChild(request);
      return response;
    },
    







    request: function(action, data, callback) {
      var request = document.createTextNode('');
      if (callback) {
        document.addEventListener('pdf.js.response', function listener(event) {
          var node = event.target,
              response = event.detail.response;

          document.documentElement.removeChild(node);

          document.removeEventListener('pdf.js.response', listener, false);
          return callback(response);
        }, false);
      }
      document.documentElement.appendChild(request);

      var sender = document.createEvent('CustomEvent');
      sender.initCustomEvent('pdf.js.message', true, false,
                             {action: action, data: data, sync: false,
                              callback: callback});
      return request.dispatchEvent(sender);
    }
  };
})();






var Settings = (function SettingsClosure() {

  function Settings(fingerprint) {
    this.fingerprint = fingerprint;
    this.initializedPromise = new PDFJS.Promise();

    var resolvePromise = (function settingsResolvePromise(db) {
      this.initialize(db || '{}');
      this.initializedPromise.resolve();
    }).bind(this);


    resolvePromise(FirefoxCom.requestSync('getDatabase', null));

  }

  Settings.prototype = {
    initialize: function settingsInitialize(database) {
      database = JSON.parse(database);
      if (!('files' in database))
        database.files = [];
      if (database.files.length >= SETTINGS_MEMORY)
        database.files.shift();
      var index;
      for (var i = 0, length = database.files.length; i < length; i++) {
        var branch = database.files[i];
        if (branch.fingerprint == this.fingerprint) {
          index = i;
          break;
        }
      }
      if (typeof index != 'number')
        index = database.files.push({fingerprint: this.fingerprint}) - 1;
      this.file = database.files[index];
      this.database = database;
    },

    set: function settingsSet(name, val) {
      if (!this.initializedPromise.isResolved)
        return;

      var file = this.file;
      file[name] = val;
      var database = JSON.stringify(this.database);


      FirefoxCom.requestSync('setDatabase', database);

    },

    get: function settingsGet(name, defaultValue) {
      if (!this.initializedPromise.isResolved)
        return defaultValue;

      return this.file[name] || defaultValue;
    }
  };

  return Settings;
})();

var cache = new Cache(CACHE_SIZE);
var currentPageNumber = 1;

var PDFFindController = {
  startedTextExtraction: false,

  extractTextPromises: [],

  
  active: false,

  
  pageContents: [],

  pageMatches: [],

  
  selected: {
    pageIdx: -1,
    matchIdx: -1
  },

  
  offset: {
    pageIdx: null,
    matchIdx: null
  },

  resumePageIdx: null,

  resumeCallback: null,

  state: null,

  dirtyMatch: false,

  findTimeout: null,

  initialize: function() {
    var events = [
      'find',
      'findagain',
      'findhighlightallchange',
      'findcasesensitivitychange'
    ];

    this.handleEvent = this.handleEvent.bind(this);

    for (var i = 0; i < events.length; i++) {
      window.addEventListener(events[i], this.handleEvent);
    }
  },

  calcFindMatch: function(pageIndex) {
    var pageContent = this.pageContents[pageIndex];
    var query = this.state.query;
    var caseSensitive = this.state.caseSensitive;
    var queryLen = query.length;

    if (queryLen === 0) {
      
      return;
    }

    if (!caseSensitive) {
      pageContent = pageContent.toLowerCase();
      query = query.toLowerCase();
    }

    var matches = [];

    var matchIdx = -queryLen;
    while (true) {
      matchIdx = pageContent.indexOf(query, matchIdx + queryLen);
      if (matchIdx === -1) {
        break;
      }

      matches.push(matchIdx);
    }
    this.pageMatches[pageIndex] = matches;
    this.updatePage(pageIndex);
    if (this.resumePageIdx === pageIndex) {
      var callback = this.resumeCallback;
      this.resumePageIdx = null;
      this.resumeCallback = null;
      callback();
    }
  },

  extractText: function() {
    if (this.startedTextExtraction) {
      return;
    }
    this.startedTextExtraction = true;

    this.pageContents = [];
    for (var i = 0, ii = PDFView.pdfDocument.numPages; i < ii; i++) {
      this.extractTextPromises.push(new PDFJS.Promise());
    }

    var self = this;
    function extractPageText(pageIndex) {
      PDFView.pages[pageIndex].getTextContent().then(
        function textContentResolved(data) {
          
          var bidiTexts = data.bidiTexts;
          var str = '';

          for (var i = 0; i < bidiTexts.length; i++) {
            str += bidiTexts[i].str;
          }

          
          self.pageContents.push(str);

          self.extractTextPromises[pageIndex].resolve(pageIndex);
          if ((pageIndex + 1) < PDFView.pages.length)
            extractPageText(pageIndex + 1);
        }
      );
    }
    extractPageText(0);
    return this.extractTextPromise;
  },

  handleEvent: function(e) {
    if (this.state === null || e.type !== 'findagain') {
      this.dirtyMatch = true;
    }
    this.state = e.detail;
    this.updateUIState(FindStates.FIND_PENDING);

    this.extractText();

    clearTimeout(this.findTimeout);
    if (e.type === 'find') {
      
      this.findTimeout = setTimeout(this.nextMatch.bind(this), 250);
    } else {
      this.nextMatch();
    }
  },

  updatePage: function(idx) {
    var page = PDFView.pages[idx];

    if (this.selected.pageIdx === idx) {
      
      
      
      page.scrollIntoView();
    }

    if (page.textLayer) {
      page.textLayer.updateMatches();
    }
  },

  nextMatch: function() {
    var pages = PDFView.pages;
    var previous = this.state.findPrevious;
    var numPages = PDFView.pages.length;

    this.active = true;

    if (this.dirtyMatch) {
      
      this.dirtyMatch = false;
      this.selected.pageIdx = this.selected.matchIdx = -1;
      this.offset.pageIdx = previous ? numPages - 1 : 0;
      this.offset.matchIdx = null;
      this.hadMatch = false;
      this.resumeCallback = null;
      this.resumePageIdx = null;
      this.pageMatches = [];
      var self = this;

      for (var i = 0; i < numPages; i++) {
        
        this.updatePage(i);

        
        this.extractTextPromises[i].onData(function(pageIdx) {
          
          
          setTimeout(function() {
            self.calcFindMatch(pageIdx);
          });
        });
      }
    }

    
    if (this.state.query === '') {
      this.updateUIState(FindStates.FIND_FOUND);
      return;
    }

    
    if (this.resumeCallback) {
      return;
    }

    var offset = this.offset;
    
    
    if (offset.matchIdx !== null) {
      var numPageMatches = this.pageMatches[offset.pageIdx].length;
      if ((!previous && offset.matchIdx + 1 < numPageMatches) ||
          (previous && offset.matchIdx > 0)) {
        
        
        this.hadMatch = true;
        offset.matchIdx = previous ? offset.matchIdx - 1 : offset.matchIdx + 1;
        this.updateMatch(true);
        return;
      }
      
      
      this.advanceOffsetPage(previous);
    }
    
    this.nextPageMatch();
  },

  nextPageMatch: function() {
    if (this.resumePageIdx !== null)
      console.error('There can only be one pending page.');

    var matchesReady = function(matches) {
      var offset = this.offset;
      var numMatches = matches.length;
      var previous = this.state.findPrevious;
      if (numMatches) {
        
        this.hadMatch = true;
        offset.matchIdx = previous ? numMatches - 1 : 0;
        this.updateMatch(true);
      } else {
        
        this.advanceOffsetPage(previous);
        if (offset.wrapped) {
          offset.matchIdx = null;
          if (!this.hadMatch) {
            
            this.updateMatch(false);
            return;
          }
        }
        
        this.nextPageMatch();
      }
    }.bind(this);

    var pageIdx = this.offset.pageIdx;
    var pageMatches = this.pageMatches;
    if (!pageMatches[pageIdx]) {
      
      
      this.resumeCallback = function() {
        matchesReady(pageMatches[pageIdx]);
      };
      this.resumePageIdx = pageIdx;
      return;
    }
    
    matchesReady(pageMatches[pageIdx]);
  },

  advanceOffsetPage: function(previous) {
    var offset = this.offset;
    var numPages = this.extractTextPromises.length;
    offset.pageIdx = previous ? offset.pageIdx - 1 : offset.pageIdx + 1;
    offset.matchIdx = null;
    if (offset.pageIdx >= numPages || offset.pageIdx < 0) {
      offset.pageIdx = previous ? numPages - 1 : 0;
      offset.wrapped = true;
      return;
    }
  },

  updateMatch: function(found) {
    var state = FindStates.FIND_NOTFOUND;
    var wrapped = this.offset.wrapped;
    this.offset.wrapped = false;
    if (found) {
      var previousPage = this.selected.pageIdx;
      this.selected.pageIdx = this.offset.pageIdx;
      this.selected.matchIdx = this.offset.matchIdx;
      state = wrapped ? FindStates.FIND_WRAPPED : FindStates.FIND_FOUND;
      
      if (previousPage !== -1 && previousPage !== this.selected.pageIdx) {
        this.updatePage(previousPage);
      }
    }
    this.updateUIState(state, this.state.findPrevious);
    if (this.selected.pageIdx !== -1) {
      this.updatePage(this.selected.pageIdx, true);
    }
  },

  updateUIState: function(state, previous) {
    if (PDFView.supportsIntegratedFind) {
      FirefoxCom.request('updateFindControlState',
                         {result: state, findPrevious: previous});
      return;
    }
    PDFFindBar.updateUIState(state, previous);
  }
};

var PDFFindBar = {
  
  

  opened: false,

  initialize: function() {
    this.bar = document.getElementById('findbar');
    this.toggleButton = document.getElementById('viewFind');
    this.findField = document.getElementById('findInput');
    this.highlightAll = document.getElementById('findHighlightAll');
    this.caseSensitive = document.getElementById('findMatchCase');
    this.findMsg = document.getElementById('findMsg');
    this.findStatusIcon = document.getElementById('findStatusIcon');

    var self = this;
    this.toggleButton.addEventListener('click', function() {
      self.toggle();
    });

    this.findField.addEventListener('input', function() {
      self.dispatchEvent('');
    });

    this.bar.addEventListener('keydown', function(evt) {
      switch (evt.keyCode) {
        case 13: 
          if (evt.target === self.findField) {
            self.dispatchEvent('again', evt.shiftKey);
          }
          break;
        case 27: 
          self.close();
          break;
      }
    });

    document.getElementById('findPrevious').addEventListener('click',
      function() { self.dispatchEvent('again', true); }
    );

    document.getElementById('findNext').addEventListener('click', function() {
      self.dispatchEvent('again', false);
    });

    this.highlightAll.addEventListener('click', function() {
      self.dispatchEvent('highlightallchange');
    });

    this.caseSensitive.addEventListener('click', function() {
      self.dispatchEvent('casesensitivitychange');
    });
  },

  dispatchEvent: function(aType, aFindPrevious) {
    var event = document.createEvent('CustomEvent');
    event.initCustomEvent('find' + aType, true, true, {
      query: this.findField.value,
      caseSensitive: this.caseSensitive.checked,
      highlightAll: this.highlightAll.checked,
      findPrevious: aFindPrevious
    });
    return window.dispatchEvent(event);
  },

  updateUIState: function(state, previous) {
    var notFound = false;
    var findMsg = '';
    var status = '';

    switch (state) {
      case FindStates.FIND_FOUND:
        break;

      case FindStates.FIND_PENDING:
        status = 'pending';
        break;

      case FindStates.FIND_NOTFOUND:
        findMsg = mozL10n.get('find_not_found', null, 'Phrase not found');
        notFound = true;
        break;

      case FindStates.FIND_WRAPPED:
        if (previous) {
          findMsg = mozL10n.get('find_reached_top', null,
                      'Reached top of document, continued from bottom');
        } else {
          findMsg = mozL10n.get('find_reached_bottom', null,
                                'Reached end of document, continued from top');
        }
        break;
    }

    if (notFound) {
      this.findField.classList.add('notFound');
    } else {
      this.findField.classList.remove('notFound');
    }

    this.findField.setAttribute('data-status', status);
    this.findMsg.textContent = findMsg;
  },

  open: function() {
    if (this.opened) return;

    this.opened = true;
    this.toggleButton.classList.add('toggled');
    this.bar.classList.remove('hidden');
    this.findField.select();
    this.findField.focus();
  },

  close: function() {
    if (!this.opened) return;

    this.opened = false;
    this.toggleButton.classList.remove('toggled');
    this.bar.classList.add('hidden');

    PDFFindController.active = false;
  },

  toggle: function() {
    if (this.opened) {
      this.close();
    } else {
      this.open();
    }
  }
};

var PDFView = {
  pages: [],
  thumbnails: [],
  currentScale: UNKNOWN_SCALE,
  currentScaleValue: null,
  initialBookmark: document.location.hash.substring(1),
  startedTextExtraction: false,
  pageText: [],
  container: null,
  thumbnailContainer: null,
  initialized: false,
  fellback: false,
  pdfDocument: null,
  sidebarOpen: false,
  pageViewScroll: null,
  thumbnailViewScroll: null,
  isFullscreen: false,
  previousScale: null,
  pageRotation: 0,
  mouseScrollTimeStamp: 0,
  mouseScrollDelta: 0,
  lastScroll: 0,
  previousPageNumber: 1,

  
  initialize: function pdfViewInitialize() {
    var self = this;
    var container = this.container = document.getElementById('viewerContainer');
    this.pageViewScroll = {};
    this.watchScroll(container, this.pageViewScroll, updateViewarea);

    var thumbnailContainer = this.thumbnailContainer =
                             document.getElementById('thumbnailView');
    this.thumbnailViewScroll = {};
    this.watchScroll(thumbnailContainer, this.thumbnailViewScroll,
                     this.renderHighestPriority.bind(this));

    PDFFindBar.initialize();
    PDFFindController.initialize();

    this.initialized = true;
    container.addEventListener('scroll', function() {
      self.lastScroll = Date.now();
    }, false);
  },

  
  
  watchScroll: function pdfViewWatchScroll(viewAreaElement, state, callback) {
    state.down = true;
    state.lastY = viewAreaElement.scrollTop;
    viewAreaElement.addEventListener('scroll', function webViewerScroll(evt) {
      var currentY = viewAreaElement.scrollTop;
      var lastY = state.lastY;
      if (currentY > lastY)
        state.down = true;
      else if (currentY < lastY)
        state.down = false;
      
      state.lastY = currentY;
      callback();
    }, true);
  },

  setScale: function pdfViewSetScale(val, resetAutoSettings, noScroll) {
    if (val == this.currentScale)
      return;

    var pages = this.pages;
    for (var i = 0; i < pages.length; i++)
      pages[i].update(val * CSS_UNITS);

    if (!noScroll && this.currentScale != val)
      this.pages[this.page - 1].scrollIntoView();
    this.currentScale = val;

    var event = document.createEvent('UIEvents');
    event.initUIEvent('scalechange', false, false, window, 0);
    event.scale = val;
    event.resetAutoSettings = resetAutoSettings;
    window.dispatchEvent(event);
  },

  parseScale: function pdfViewParseScale(value, resetAutoSettings, noScroll) {
    if ('custom' == value)
      return;

    var scale = parseFloat(value);
    this.currentScaleValue = value;
    if (scale) {
      this.setScale(scale, true, noScroll);
      return;
    }

    var container = this.container;
    var currentPage = this.pages[this.page - 1];
    if (!currentPage) {
      return;
    }

    var pageWidthScale = (container.clientWidth - SCROLLBAR_PADDING) /
                          currentPage.width * currentPage.scale / CSS_UNITS;
    var pageHeightScale = (container.clientHeight - VERTICAL_PADDING) /
                           currentPage.height * currentPage.scale / CSS_UNITS;
    switch (value) {
      case 'page-actual':
        scale = 1;
        break;
      case 'page-width':
        scale = pageWidthScale;
        break;
      case 'page-height':
        scale = pageHeightScale;
        break;
      case 'page-fit':
        scale = Math.min(pageWidthScale, pageHeightScale);
        break;
      case 'auto':
        scale = Math.min(1.0, pageWidthScale);
        break;
    }
    this.setScale(scale, resetAutoSettings, noScroll);

    selectScaleOption(value);
  },

  zoomIn: function pdfViewZoomIn() {
    var newScale = (this.currentScale * DEFAULT_SCALE_DELTA).toFixed(2);
    newScale = Math.ceil(newScale * 10) / 10;
    newScale = Math.min(MAX_SCALE, newScale);
    this.parseScale(newScale, true);
  },

  zoomOut: function pdfViewZoomOut() {
    var newScale = (this.currentScale / DEFAULT_SCALE_DELTA).toFixed(2);
    newScale = Math.floor(newScale * 10) / 10;
    newScale = Math.max(MIN_SCALE, newScale);
    this.parseScale(newScale, true);
  },

  set page(val) {
    var pages = this.pages;
    var input = document.getElementById('pageNumber');
    var event = document.createEvent('UIEvents');
    event.initUIEvent('pagechange', false, false, window, 0);

    if (!(0 < val && val <= pages.length)) {
      this.previousPageNumber = val;
      event.pageNumber = this.page;
      window.dispatchEvent(event);
      return;
    }

    pages[val - 1].updateStats();
    this.previousPageNumber = currentPageNumber;
    currentPageNumber = val;
    event.pageNumber = val;
    window.dispatchEvent(event);

    
    
    if (updateViewarea.inProgress)
      return;

    
    if (this.loading && val == 1)
      return;

    pages[val - 1].scrollIntoView();
  },

  get page() {
    return currentPageNumber;
  },

  get supportsPrinting() {
    var canvas = document.createElement('canvas');
    var value = 'mozPrintCallback' in canvas;
    
    Object.defineProperty(this, 'supportsPrinting', { value: value,
                                                      enumerable: true,
                                                      configurable: true,
                                                      writable: false });
    return value;
  },

  get supportsFullscreen() {
    var doc = document.documentElement;
    var support = doc.requestFullscreen || doc.mozRequestFullScreen ||
                  doc.webkitRequestFullScreen;

    
    if (!!window.frameElement)
      support = false;

    Object.defineProperty(this, 'supportsFullScreen', { value: support,
                                                        enumerable: true,
                                                        configurable: true,
                                                        writable: false });
    return support;
  },

  get supportsIntegratedFind() {
    var support = false;
    support = FirefoxCom.requestSync('supportsIntegratedFind');
    Object.defineProperty(this, 'supportsIntegratedFind', { value: support,
                                                            enumerable: true,
                                                            configurable: true,
                                                            writable: false });
    return support;
  },

  get supportsDocumentFonts() {
    var support = true;
    support = FirefoxCom.requestSync('supportsDocumentFonts');
    Object.defineProperty(this, 'supportsDocumentFonts', { value: support,
                                                           enumerable: true,
                                                           configurable: true,
                                                           writable: false });
    return support;
  },

  get isHorizontalScrollbarEnabled() {
    var div = document.getElementById('viewerContainer');
    return div.scrollWidth > div.clientWidth;
  },

  initPassiveLoading: function pdfViewInitPassiveLoading() {
    if (!PDFView.loadingBar) {
      PDFView.loadingBar = new ProgressBar('#loadingBar', {});
    }

    window.addEventListener('message', function window_message(e) {
      var args = e.data;

      if (typeof args !== 'object' || !('pdfjsLoadAction' in args))
        return;
      switch (args.pdfjsLoadAction) {
        case 'progress':
          PDFView.progress(args.loaded / args.total);
          break;
        case 'complete':
          if (!args.data) {
            PDFView.error(mozL10n.get('loading_error', null,
                          'An error occurred while loading the PDF.'), e);
            break;
          }
          PDFView.open(args.data, 0);
          break;
      }
    });
    FirefoxCom.requestSync('initPassiveLoading', null);
  },

  setTitleUsingUrl: function pdfViewSetTitleUsingUrl(url) {
    this.url = url;
    try {
      this.setTitle(decodeURIComponent(getFileName(url)) || url);
    } catch (e) {
      
      
      this.setTitle(url);
    }
  },

  setTitle: function pdfViewSetTitle(title) {
    document.title = title;
  },

  open: function pdfViewOpen(url, scale, password) {
    var parameters = {password: password};
    if (typeof url === 'string') { 
      this.setTitleUsingUrl(url);
      parameters.url = url;
    } else if (url && 'byteLength' in url) { 
      parameters.data = url;
    }

    if (!PDFView.loadingBar) {
      PDFView.loadingBar = new ProgressBar('#loadingBar', {});
    }

    this.pdfDocument = null;
    var self = this;
    self.loading = true;
    PDFJS.getDocument(parameters).then(
      function getDocumentCallback(pdfDocument) {
        self.load(pdfDocument, scale);
        self.loading = false;
      },
      function getDocumentError(message, exception) {
        if (exception && exception.name === 'PasswordException') {
          if (exception.code === 'needpassword') {
            var promptString = mozL10n.get('request_password', null,
                                      'PDF is protected by a password:');
            password = prompt(promptString);
            if (password && password.length > 0) {
              return PDFView.open(url, scale, password);
            }
          }
        }

        var loadingErrorMessage = mozL10n.get('loading_error', null,
          'An error occurred while loading the PDF.');

        if (exception && exception.name === 'InvalidPDFException') {
          
          var loadingErrorMessage = mozL10n.get('invalid_file_error', null,
                                        'Invalid or corrupted PDF file.');
        }

        if (exception && exception.name === 'MissingPDFException') {
          
          var loadingErrorMessage = mozL10n.get('missing_file_error', null,
                                        'Missing PDF file.');

        }

        var loadingIndicator = document.getElementById('loading');
        loadingIndicator.textContent = mozL10n.get('loading_error_indicator',
          null, 'Error');
        var moreInfo = {
          message: message
        };
        self.error(loadingErrorMessage, moreInfo);
        self.loading = false;
      },
      function getDocumentProgress(progressData) {
        self.progress(progressData.loaded / progressData.total);
      }
    );
  },

  download: function pdfViewDownload() {
    function noData() {
      FirefoxCom.request('download', { originalUrl: url });
    }
    var url = this.url.split('#')[0];
    
    if (!this.pdfDocument) {
      noData();
      return;
    }
    this.pdfDocument.getData().then(
      function getDataSuccess(data) {
        var blob = PDFJS.createBlob(data.buffer, 'application/pdf');
        var blobUrl = window.URL.createObjectURL(blob);
  
        FirefoxCom.request('download', { blobUrl: blobUrl, originalUrl: url },
          function response(err) {
            if (err) {
              
              
              PDFView.error('PDF failed to download.');
            }
            window.URL.revokeObjectURL(blobUrl);
          }
        );
      },
      noData 
    );
  },

  fallback: function pdfViewFallback() {
    
    
    if (this.fellback)
      return;
    this.fellback = true;
    var url = this.url.split('#')[0];
    FirefoxCom.request('fallback', url, function response(download) {
      if (!download)
        return;
      PDFView.download();
    });
  },

  navigateTo: function pdfViewNavigateTo(dest) {
    if (typeof dest === 'string')
      dest = this.destinations[dest];
    if (!(dest instanceof Array))
      return; 
    
    var destRef = dest[0];
    var pageNumber = destRef instanceof Object ?
      this.pagesRefMap[destRef.num + ' ' + destRef.gen + ' R'] : (destRef + 1);
    if (pageNumber > this.pages.length)
      pageNumber = this.pages.length;
    if (pageNumber) {
      this.page = pageNumber;
      var currentPage = this.pages[pageNumber - 1];
      if (!this.isFullscreen) { 
        currentPage.scrollIntoView(dest);
      }
    }
  },

  getDestinationHash: function pdfViewGetDestinationHash(dest) {
    if (typeof dest === 'string')
      return PDFView.getAnchorUrl('#' + escape(dest));
    if (dest instanceof Array) {
      var destRef = dest[0]; 
      var pageNumber = destRef instanceof Object ?
        this.pagesRefMap[destRef.num + ' ' + destRef.gen + ' R'] :
        (destRef + 1);
      if (pageNumber) {
        var pdfOpenParams = PDFView.getAnchorUrl('#page=' + pageNumber);
        var destKind = dest[1];
        if (typeof destKind === 'object' && 'name' in destKind &&
            destKind.name == 'XYZ') {
          var scale = (dest[4] || this.currentScale);
          pdfOpenParams += '&zoom=' + (scale * 100);
          if (dest[2] || dest[3]) {
            pdfOpenParams += ',' + (dest[2] || 0) + ',' + (dest[3] || 0);
          }
        }
        return pdfOpenParams;
      }
    }
    return '';
  },

  




  getAnchorUrl: function getAnchorUrl(anchor) {
    return this.url.split('#')[0] + anchor;
  },

  





  getOutputScale: function pdfViewGetOutputDPI() {
    var pixelRatio = 'devicePixelRatio' in window ? window.devicePixelRatio : 1;
    return {
      sx: pixelRatio,
      sy: pixelRatio,
      scaled: pixelRatio != 1
    };
  },

  






  error: function pdfViewError(message, moreInfo) {
    var moreInfoText = mozL10n.get('error_version_info',
      {version: PDFJS.version || '?', build: PDFJS.build || '?'},
      'PDF.js v{{version}} (build: {{build}})') + '\n';
    if (moreInfo) {
      moreInfoText +=
        mozL10n.get('error_message', {message: moreInfo.message},
        'Message: {{message}}');
      if (moreInfo.stack) {
        moreInfoText += '\n' +
          mozL10n.get('error_stack', {stack: moreInfo.stack},
          'Stack: {{stack}}');
      } else {
        if (moreInfo.filename) {
          moreInfoText += '\n' +
            mozL10n.get('error_file', {file: moreInfo.filename},
            'File: {{file}}');
        }
        if (moreInfo.lineNumber) {
          moreInfoText += '\n' +
            mozL10n.get('error_line', {line: moreInfo.lineNumber},
            'Line: {{line}}');
        }
      }
    }

    var loadingBox = document.getElementById('loadingBox');
    loadingBox.setAttribute('hidden', 'true');

    console.error(message + '\n' + moreInfoText);
    this.fallback();
  },

  progress: function pdfViewProgress(level) {
    var percent = Math.round(level * 100);
    PDFView.loadingBar.percent = percent;
  },

  load: function pdfViewLoad(pdfDocument, scale) {
    function bindOnAfterDraw(pageView, thumbnailView) {
      
      pageView.onAfterDraw = function pdfViewLoadOnAfterDraw() {
        thumbnailView.setImage(pageView.canvas);
      };
    }

    this.pdfDocument = pdfDocument;

    var errorWrapper = document.getElementById('errorWrapper');
    errorWrapper.setAttribute('hidden', 'true');

    var loadingBox = document.getElementById('loadingBox');
    loadingBox.setAttribute('hidden', 'true');
    var loadingIndicator = document.getElementById('loading');
    loadingIndicator.textContent = '';

    var thumbsView = document.getElementById('thumbnailView');
    thumbsView.parentNode.scrollTop = 0;

    while (thumbsView.hasChildNodes())
      thumbsView.removeChild(thumbsView.lastChild);

    if ('_loadingInterval' in thumbsView)
      clearInterval(thumbsView._loadingInterval);

    var container = document.getElementById('viewer');
    while (container.hasChildNodes())
      container.removeChild(container.lastChild);

    var pagesCount = pdfDocument.numPages;
    var id = pdfDocument.fingerprint;
    document.getElementById('numPages').textContent =
      mozL10n.get('page_of', {pageCount: pagesCount}, 'of {{pageCount}}');
    document.getElementById('pageNumber').max = pagesCount;

    PDFView.documentFingerprint = id;
    var store = PDFView.store = new Settings(id);
    var storePromise = store.initializedPromise;

    this.pageRotation = 0;

    var pages = this.pages = [];
    this.pageText = [];
    this.startedTextExtraction = false;
    var pagesRefMap = {};
    var thumbnails = this.thumbnails = [];
    var pagePromises = [];
    for (var i = 1; i <= pagesCount; i++)
      pagePromises.push(pdfDocument.getPage(i));
    var self = this;
    var pagesPromise = PDFJS.Promise.all(pagePromises);
    pagesPromise.then(function(promisedPages) {
      for (var i = 1; i <= pagesCount; i++) {
        var page = promisedPages[i - 1];
        var pageView = new PageView(container, page, i, scale,
                                    page.stats, self.navigateTo.bind(self));
        var thumbnailView = new ThumbnailView(thumbsView, page, i);
        bindOnAfterDraw(pageView, thumbnailView);

        pages.push(pageView);
        thumbnails.push(thumbnailView);
        var pageRef = page.ref;
        pagesRefMap[pageRef.num + ' ' + pageRef.gen + ' R'] = i;
      }

      self.pagesRefMap = pagesRefMap;

      
      if (PDFView.supportsPrinting) {
        pdfDocument.getJavaScript().then(function(javaScript) {
          if (javaScript.length) {
            console.warn('Warning: JavaScript is not supported');
            PDFView.fallback();
          }
          
          var regex = /\bprint\s*\(/g;
          for (var i = 0, ii = javaScript.length; i < ii; i++) {
            var js = javaScript[i];
            if (js && regex.test(js)) {
              setTimeout(function() {
                window.print();
              });
              return;
            }
          }
        });
      }
    });

    var destinationsPromise = pdfDocument.getDestinations();
    destinationsPromise.then(function(destinations) {
      self.destinations = destinations;
    });

    
    var promises = [pagesPromise, destinationsPromise, storePromise,
                    PDFView.animationStartedPromise];
    PDFJS.Promise.all(promises).then(function() {
      pdfDocument.getOutline().then(function(outline) {
        self.outline = new DocumentOutlineView(outline);
      });

      var storedHash = null;
      if (store.get('exists', false)) {
        var page = store.get('page', '1');
        var zoom = store.get('zoom', PDFView.currentScale);
        var left = store.get('scrollLeft', '0');
        var top = store.get('scrollTop', '0');

        storedHash = 'page=' + page + '&zoom=' + zoom + ',' + left + ',' + top;
      }

      self.setInitialView(storedHash, scale);

      
      
      if (window.parent.location === window.location) {
        PDFView.container.focus();
      }
    });

    pdfDocument.getMetadata().then(function(data) {
      var info = data.info, metadata = data.metadata;
      self.documentInfo = info;
      self.metadata = metadata;

      
      console.log('PDF ' + pdfDocument.fingerprint + ' [' +
                  info.PDFFormatVersion + ' ' + (info.Producer || '-') +
                  ' / ' + (info.Creator || '-') + ']' +
                  (PDFJS.version ? ' (PDF.js: ' + PDFJS.version + ')' : ''));

      var pdfTitle;
      if (metadata) {
        if (metadata.has('dc:title'))
          pdfTitle = metadata.get('dc:title');
      }

      if (!pdfTitle && info && info['Title'])
        pdfTitle = info['Title'];

      if (pdfTitle)
        self.setTitle(pdfTitle + ' - ' + document.title);

      if (info.IsAcroFormPresent) {
        console.warn('Warning: AcroForm/XFA is not supported');
        PDFView.fallback();
      }
    });
  },

  setInitialView: function pdfViewSetInitialView(storedHash, scale) {
    
    
    this.currentScale = 0;
    this.currentScaleValue = null;
    if (this.initialBookmark) {
      this.setHash(this.initialBookmark);
      this.initialBookmark = null;
    }
    else if (storedHash)
      this.setHash(storedHash);
    else if (scale) {
      this.parseScale(scale, true);
      this.page = 1;
    }

    if (PDFView.currentScale === UNKNOWN_SCALE) {
      
      
      this.parseScale(DEFAULT_SCALE, true);
    }
  },

  renderHighestPriority: function pdfViewRenderHighestPriority() {
    
    var visiblePages = this.getVisiblePages();
    var pageView = this.getHighestPriority(visiblePages, this.pages,
                                           this.pageViewScroll.down);
    if (pageView) {
      this.renderView(pageView, 'page');
      return;
    }
    
    if (this.sidebarOpen) {
      var visibleThumbs = this.getVisibleThumbs();
      var thumbView = this.getHighestPriority(visibleThumbs,
                                              this.thumbnails,
                                              this.thumbnailViewScroll.down);
      if (thumbView)
        this.renderView(thumbView, 'thumbnail');
    }
  },

  getHighestPriority: function pdfViewGetHighestPriority(visible, views,
                                                         scrolledDown) {
    
    
    
    
    
    
    var visibleViews = visible.views;

    var numVisible = visibleViews.length;
    if (numVisible === 0) {
      return false;
    }
    for (var i = 0; i < numVisible; ++i) {
      var view = visibleViews[i].view;
      if (!this.isViewFinished(view))
        return view;
    }

    
    if (scrolledDown) {
      var nextPageIndex = visible.last.id;
      
      if (views[nextPageIndex] && !this.isViewFinished(views[nextPageIndex]))
        return views[nextPageIndex];
    } else {
      var previousPageIndex = visible.first.id - 2;
      if (views[previousPageIndex] &&
          !this.isViewFinished(views[previousPageIndex]))
        return views[previousPageIndex];
    }
    
    return false;
  },

  isViewFinished: function pdfViewNeedsRendering(view) {
    return view.renderingState === RenderingStates.FINISHED;
  },

  
  
  renderView: function pdfViewRender(view, type) {
    var state = view.renderingState;
    switch (state) {
      case RenderingStates.FINISHED:
        return false;
      case RenderingStates.PAUSED:
        PDFView.highestPriorityPage = type + view.id;
        view.resume();
        break;
      case RenderingStates.RUNNING:
        PDFView.highestPriorityPage = type + view.id;
        break;
      case RenderingStates.INITIAL:
        PDFView.highestPriorityPage = type + view.id;
        view.draw(this.renderHighestPriority.bind(this));
        break;
    }
    return true;
  },

  setHash: function pdfViewSetHash(hash) {
    if (!hash)
      return;

    if (hash.indexOf('=') >= 0) {
      var params = PDFView.parseQueryString(hash);
      
      if ('nameddest' in params) {
        PDFView.navigateTo(params.nameddest);
        return;
      }
      if ('page' in params) {
        var pageNumber = (params.page | 0) || 1;
        if ('zoom' in params) {
          var zoomArgs = params.zoom.split(','); 
          

          
          
          var zoomArg = zoomArgs[0];
          var zoomArgNumber = parseFloat(zoomArg);
          if (zoomArgNumber)
            zoomArg = zoomArgNumber / 100;

          var dest = [null, {name: 'XYZ'},
                      zoomArgs.length > 1 ? (zoomArgs[1] | 0) : null,
                      zoomArgs.length > 2 ? (zoomArgs[2] | 0) : null,
                      zoomArg];
          var currentPage = this.pages[pageNumber - 1];
          currentPage.scrollIntoView(dest);
        } else {
          this.page = pageNumber; 
        }
      }
      if ('pagemode' in params) {
        var toggle = document.getElementById('sidebarToggle');
        if (params.pagemode === 'thumbs' || params.pagemode === 'bookmarks') {
          if (!this.sidebarOpen) {
            toggle.click();
          }
          this.switchSidebarView(params.pagemode === 'thumbs' ?
                                 'thumbs' : 'outline');
        } else if (params.pagemode === 'none' && this.sidebarOpen) {
          toggle.click();
        }
      }
    } else if (/^\d+$/.test(hash)) 
      this.page = hash;
    else 
      PDFView.navigateTo(unescape(hash));
  },

  switchSidebarView: function pdfViewSwitchSidebarView(view) {
    var thumbsView = document.getElementById('thumbnailView');
    var outlineView = document.getElementById('outlineView');

    var thumbsButton = document.getElementById('viewThumbnail');
    var outlineButton = document.getElementById('viewOutline');

    switch (view) {
      case 'thumbs':
        var wasOutlineViewVisible = thumbsView.classList.contains('hidden');

        thumbsButton.classList.add('toggled');
        outlineButton.classList.remove('toggled');
        thumbsView.classList.remove('hidden');
        outlineView.classList.add('hidden');

        PDFView.renderHighestPriority();

        if (wasOutlineViewVisible) {
          
          
          scrollIntoView(document.getElementById('thumbnailContainer' +
                                                 this.page));
        }
        break;

      case 'outline':
        thumbsButton.classList.remove('toggled');
        outlineButton.classList.add('toggled');
        thumbsView.classList.add('hidden');
        outlineView.classList.remove('hidden');

        if (outlineButton.getAttribute('disabled'))
          return;
        break;
    }
  },

  getVisiblePages: function pdfViewGetVisiblePages() {
    if (!this.isFullscreen) {
      return this.getVisibleElements(this.container, this.pages, true);
    } else {
      
      var visible = [], page = this.page;
      var currentPage = this.pages[page - 1];
      visible.push({ id: currentPage.id, view: currentPage });

      return { first: currentPage, last: currentPage, views: visible};
    }
  },

  getVisibleThumbs: function pdfViewGetVisibleThumbs() {
    return this.getVisibleElements(this.thumbnailContainer, this.thumbnails);
  },

  
  getVisibleElements: function pdfViewGetVisibleElements(
      scrollEl, views, sortByVisibility) {
    var top = scrollEl.scrollTop, bottom = top + scrollEl.clientHeight;
    var left = scrollEl.scrollLeft, right = left + scrollEl.clientWidth;

    var visible = [], view;
    var currentHeight, viewHeight, hiddenHeight, percentHeight;
    var currentWidth, viewWidth;
    for (var i = 0, ii = views.length; i < ii; ++i) {
      view = views[i];
      currentHeight = view.el.offsetTop + view.el.clientTop;
      viewHeight = view.el.clientHeight;
      if ((currentHeight + viewHeight) < top) {
        continue;
      }
      if (currentHeight > bottom) {
        break;
      }
      currentWidth = view.el.offsetLeft + view.el.clientLeft;
      viewWidth = view.el.clientWidth;
      if ((currentWidth + viewWidth) < left || currentWidth > right) {
        continue;
      }
      hiddenHeight = Math.max(0, top - currentHeight) +
                     Math.max(0, currentHeight + viewHeight - bottom);
      percentHeight = ((viewHeight - hiddenHeight) * 100 / viewHeight) | 0;

      visible.push({ id: view.id, y: currentHeight,
                     view: view, percent: percentHeight });
    }

    var first = visible[0];
    var last = visible[visible.length - 1];

    if (sortByVisibility) {
      visible.sort(function(a, b) {
        var pc = a.percent - b.percent;
        if (Math.abs(pc) > 0.001) {
          return -pc;
        }
        return a.id - b.id; 
      });
    }
    return {first: first, last: last, views: visible};
  },

  
  parseQueryString: function pdfViewParseQueryString(query) {
    var parts = query.split('&');
    var params = {};
    for (var i = 0, ii = parts.length; i < parts.length; ++i) {
      var param = parts[i].split('=');
      var key = param[0];
      var value = param.length > 1 ? param[1] : null;
      params[unescape(key)] = unescape(value);
    }
    return params;
  },

  beforePrint: function pdfViewSetupBeforePrint() {
    if (!this.supportsPrinting) {
      var printMessage = mozL10n.get('printing_not_supported', null,
          'Warning: Printing is not fully supported by this browser.');
      this.error(printMessage);
      return;
    }
    var body = document.querySelector('body');
    body.setAttribute('data-mozPrintCallback', true);
    for (var i = 0, ii = this.pages.length; i < ii; ++i) {
      this.pages[i].beforePrint();
    }
  },

  afterPrint: function pdfViewSetupAfterPrint() {
    var div = document.getElementById('printContainer');
    while (div.hasChildNodes())
      div.removeChild(div.lastChild);
  },

  fullscreen: function pdfViewFullscreen() {
    var isFullscreen = document.fullscreenElement || document.mozFullScreen ||
        document.webkitIsFullScreen;

    if (isFullscreen) {
      return false;
    }

    var wrapper = document.getElementById('viewerContainer');
    if (document.documentElement.requestFullscreen) {
      wrapper.requestFullscreen();
    } else if (document.documentElement.mozRequestFullScreen) {
      wrapper.mozRequestFullScreen();
    } else if (document.documentElement.webkitRequestFullScreen) {
      wrapper.webkitRequestFullScreen(Element.ALLOW_KEYBOARD_INPUT);
    } else {
      return false;
    }

    this.isFullscreen = true;
    var currentPage = this.pages[this.page - 1];
    this.previousScale = this.currentScaleValue;
    this.parseScale('page-fit', true);

    
    setTimeout(function() {
      currentPage.scrollIntoView();
    }, 0);

    this.showPresentationControls();
    return true;
  },

  exitFullscreen: function pdfViewExitFullscreen() {
    this.isFullscreen = false;
    this.parseScale(this.previousScale);
    this.page = this.page;
    this.clearMouseScrollState();
    this.hidePresentationControls();

    
    
    scrollIntoView(document.getElementById('thumbnailContainer' + this.page));
  },

  showPresentationControls: function pdfViewShowPresentationControls() {
    var DELAY_BEFORE_HIDING_CONTROLS = 3000;
    var wrapper = document.getElementById('viewerContainer');
    if (this.presentationControlsTimeout) {
      clearTimeout(this.presentationControlsTimeout);
    } else {
      wrapper.classList.add('presentationControls');
    }
    this.presentationControlsTimeout = setTimeout(function hideControls() {
      wrapper.classList.remove('presentationControls');
      delete PDFView.presentationControlsTimeout;
    }, DELAY_BEFORE_HIDING_CONTROLS);
  },

  hidePresentationControls: function pdfViewShowPresentationControls() {
    if (!this.presentationControlsTimeout) {
      return;
    }
    clearTimeout(this.presentationControlsTimeout);
    delete this.presentationControlsTimeout;

    var wrapper = document.getElementById('viewerContainer');
    wrapper.classList.remove('presentationControls');
  },

  rotatePages: function pdfViewPageRotation(delta) {

    this.pageRotation = (this.pageRotation + 360 + delta) % 360;

    for (var i = 0, l = this.pages.length; i < l; i++) {
      var page = this.pages[i];
      page.update(page.scale, this.pageRotation);
    }

    for (var i = 0, l = this.thumbnails.length; i < l; i++) {
      var thumb = this.thumbnails[i];
      thumb.updateRotation(this.pageRotation);
    }

    var currentPage = this.pages[this.page - 1];

    this.parseScale(this.currentScaleValue, true);

    this.renderHighestPriority();

    
    setTimeout(function() {
      currentPage.scrollIntoView();
    }, 0);
  },

  






  mouseScroll: function pdfViewMouseScroll(mouseScrollDelta) {
    var MOUSE_SCROLL_COOLDOWN_TIME = 50;

    var currentTime = (new Date()).getTime();
    var storedTime = this.mouseScrollTimeStamp;

    
    
    if (currentTime > storedTime &&
        currentTime - storedTime < MOUSE_SCROLL_COOLDOWN_TIME)
      return;

    
    
    if ((this.mouseScrollDelta > 0 && mouseScrollDelta < 0) ||
        (this.mouseScrollDelta < 0 && mouseScrollDelta > 0))
      this.clearMouseScrollState();

    this.mouseScrollDelta += mouseScrollDelta;

    var PAGE_FLIP_THRESHOLD = 120;
    if (Math.abs(this.mouseScrollDelta) >= PAGE_FLIP_THRESHOLD) {

      var PageFlipDirection = {
        UP: -1,
        DOWN: 1
      };

      
      var pageFlipDirection = (this.mouseScrollDelta > 0) ?
                                PageFlipDirection.UP :
                                PageFlipDirection.DOWN;
      this.clearMouseScrollState();
      var currentPage = this.page;

      
      
      if ((currentPage == 1 && pageFlipDirection == PageFlipDirection.UP) ||
          (currentPage == this.pages.length &&
           pageFlipDirection == PageFlipDirection.DOWN))
        return;

      this.page += pageFlipDirection;
      this.mouseScrollTimeStamp = currentTime;
    }
  },

  





  clearMouseScrollState: function pdfViewClearMouseScrollState() {
    this.mouseScrollTimeStamp = 0;
    this.mouseScrollDelta = 0;
  }
};

var PageView = function pageView(container, pdfPage, id, scale,
                                 stats, navigateTo) {
  this.id = id;
  this.pdfPage = pdfPage;

  this.rotation = 0;
  this.scale = scale || 1.0;
  this.viewport = this.pdfPage.getViewport(this.scale, this.pdfPage.rotate);

  this.renderingState = RenderingStates.INITIAL;
  this.resume = null;

  this.textContent = null;
  this.textLayer = null;

  var anchor = document.createElement('a');
  anchor.name = '' + this.id;

  var div = this.el = document.createElement('div');
  div.id = 'pageContainer' + this.id;
  div.className = 'page';
  div.style.width = Math.floor(this.viewport.width) + 'px';
  div.style.height = Math.floor(this.viewport.height) + 'px';

  container.appendChild(anchor);
  container.appendChild(div);

  this.destroy = function pageViewDestroy() {
    this.update();
    this.pdfPage.destroy();
  };

  this.update = function pageViewUpdate(scale, rotation) {
    this.renderingState = RenderingStates.INITIAL;
    this.resume = null;

    if (typeof rotation !== 'undefined') {
      this.rotation = rotation;
    }

    this.scale = scale || this.scale;

    var totalRotation = (this.rotation + this.pdfPage.rotate) % 360;
    var viewport = this.pdfPage.getViewport(this.scale, totalRotation);

    this.viewport = viewport;
    div.style.width = Math.floor(viewport.width) + 'px';
    div.style.height = Math.floor(viewport.height) + 'px';

    while (div.hasChildNodes())
      div.removeChild(div.lastChild);
    div.removeAttribute('data-loaded');

    delete this.canvas;

    this.loadingIconDiv = document.createElement('div');
    this.loadingIconDiv.className = 'loadingIcon';
    div.appendChild(this.loadingIconDiv);
  };

  Object.defineProperty(this, 'width', {
    get: function PageView_getWidth() {
      return this.viewport.width;
    },
    enumerable: true
  });

  Object.defineProperty(this, 'height', {
    get: function PageView_getHeight() {
      return this.viewport.height;
    },
    enumerable: true
  });

  function setupAnnotations(pdfPage, viewport) {
    function bindLink(link, dest) {
      link.href = PDFView.getDestinationHash(dest);
      link.onclick = function pageViewSetupLinksOnclick() {
        if (dest)
          PDFView.navigateTo(dest);
        return false;
      };
    }
    function createElementWithStyle(tagName, item, rect) {
      if (!rect) {
        rect = viewport.convertToViewportRectangle(item.rect);
        rect = PDFJS.Util.normalizeRect(rect);
      }
      var element = document.createElement(tagName);
      element.style.left = Math.floor(rect[0]) + 'px';
      element.style.top = Math.floor(rect[1]) + 'px';
      element.style.width = Math.ceil(rect[2] - rect[0]) + 'px';
      element.style.height = Math.ceil(rect[3] - rect[1]) + 'px';
      return element;
    }
    function createTextAnnotation(item) {
      var container = document.createElement('section');
      container.className = 'annotText';

      var rect = viewport.convertToViewportRectangle(item.rect);
      rect = PDFJS.Util.normalizeRect(rect);
      
      if ((rect[3] - rect[1]) < ANNOT_MIN_SIZE) {
        rect[3] = rect[1] + ANNOT_MIN_SIZE;
      }
      if ((rect[2] - rect[0]) < ANNOT_MIN_SIZE) {
        rect[2] = rect[0] + (rect[3] - rect[1]); 
      }
      var image = createElementWithStyle('img', item, rect);
      var iconName = item.name;
      image.src = IMAGE_DIR + 'annotation-' +
        iconName.toLowerCase() + '.svg';
      image.alt = mozL10n.get('text_annotation_type', {type: iconName},
        '[{{type}} Annotation]');
      var content = document.createElement('div');
      content.setAttribute('hidden', true);
      var title = document.createElement('h1');
      var text = document.createElement('p');
      content.style.left = Math.floor(rect[2]) + 'px';
      content.style.top = Math.floor(rect[1]) + 'px';
      title.textContent = item.title;

      if (!item.content && !item.title) {
        content.setAttribute('hidden', true);
      } else {
        var e = document.createElement('span');
        var lines = item.content.split(/(?:\r\n?|\n)/);
        for (var i = 0, ii = lines.length; i < ii; ++i) {
          var line = lines[i];
          e.appendChild(document.createTextNode(line));
          if (i < (ii - 1))
            e.appendChild(document.createElement('br'));
        }
        text.appendChild(e);
        image.addEventListener('mouseover', function annotationImageOver() {
           content.removeAttribute('hidden');
        }, false);

        image.addEventListener('mouseout', function annotationImageOut() {
           content.setAttribute('hidden', true);
        }, false);
      }

      content.appendChild(title);
      content.appendChild(text);
      container.appendChild(image);
      container.appendChild(content);

      return container;
    }

    pdfPage.getAnnotations().then(function(items) {
      for (var i = 0; i < items.length; i++) {
        var item = items[i];
        switch (item.type) {
          case 'Link':
            var link = createElementWithStyle('a', item);
            link.href = item.url || '';
            if (!item.url)
              bindLink(link, ('dest' in item) ? item.dest : null);
            div.appendChild(link);
            break;
          case 'Text':
            var textAnnotation = createTextAnnotation(item);
            if (textAnnotation)
              div.appendChild(textAnnotation);
            break;
        }
      }
    });
  }

  this.getPagePoint = function pageViewGetPagePoint(x, y) {
    return this.viewport.convertToPdfPoint(x, y);
  };

  this.scrollIntoView = function pageViewScrollIntoView(dest) {
      if (!dest) {
        scrollIntoView(div);
        return;
      }

      var x = 0, y = 0;
      var width = 0, height = 0, widthScale, heightScale;
      var scale = 0;
      switch (dest[1].name) {
        case 'XYZ':
          x = dest[2];
          y = dest[3];
          scale = dest[4];
          
          
          
          
          x = x !== null ? x : 0;
          y = y !== null ? y : this.height / this.scale;
          break;
        case 'Fit':
        case 'FitB':
          scale = 'page-fit';
          break;
        case 'FitH':
        case 'FitBH':
          y = dest[2];
          scale = 'page-width';
          break;
        case 'FitV':
        case 'FitBV':
          x = dest[2];
          scale = 'page-height';
          break;
        case 'FitR':
          x = dest[2];
          y = dest[3];
          width = dest[4] - x;
          height = dest[5] - y;
          widthScale = (this.container.clientWidth - SCROLLBAR_PADDING) /
            width / CSS_UNITS;
          heightScale = (this.container.clientHeight - SCROLLBAR_PADDING) /
            height / CSS_UNITS;
          scale = Math.min(widthScale, heightScale);
          break;
        default:
          return;
      }

      if (scale && scale !== PDFView.currentScale)
        PDFView.parseScale(scale, true, true);
      else if (PDFView.currentScale === UNKNOWN_SCALE)
        PDFView.parseScale(DEFAULT_SCALE, true, true);

      var boundingRect = [
        this.viewport.convertToViewportPoint(x, y),
        this.viewport.convertToViewportPoint(x + width, y + height)
      ];
      setTimeout(function pageViewScrollIntoViewRelayout() {
        
        var scale = PDFView.currentScale;
        var x = Math.min(boundingRect[0][0], boundingRect[1][0]);
        var y = Math.min(boundingRect[0][1], boundingRect[1][1]);
        var width = Math.abs(boundingRect[0][0] - boundingRect[1][0]);
        var height = Math.abs(boundingRect[0][1] - boundingRect[1][1]);

        scrollIntoView(div, {left: x, top: y, width: width, height: height});
      }, 0);
  };

  this.getTextContent = function pageviewGetTextContent() {
    if (!this.textContent) {
      this.textContent = this.pdfPage.getTextContent();
    }
    return this.textContent;
  };

  this.draw = function pageviewDraw(callback) {
    if (this.renderingState !== RenderingStates.INITIAL) {
      console.error('Must be in new state before drawing');
    }

    this.renderingState = RenderingStates.RUNNING;

    var canvas = document.createElement('canvas');
    canvas.id = 'page' + this.id;
    div.appendChild(canvas);
    this.canvas = canvas;

    var textLayerDiv = null;
    if (!PDFJS.disableTextLayer) {
      textLayerDiv = document.createElement('div');
      textLayerDiv.className = 'textLayer';
      div.appendChild(textLayerDiv);
    }
    var textLayer = this.textLayer =
          textLayerDiv ? new TextLayerBuilder(textLayerDiv, this.id - 1) : null;

    var scale = this.scale, viewport = this.viewport;
    var outputScale = PDFView.getOutputScale();
    canvas.width = Math.floor(viewport.width) * outputScale.sx;
    canvas.height = Math.floor(viewport.height) * outputScale.sy;

    if (outputScale.scaled) {
      var cssScale = 'scale(' + (1 / outputScale.sx) + ', ' +
                                (1 / outputScale.sy) + ')';
      CustomStyle.setProp('transform' , canvas, cssScale);
      CustomStyle.setProp('transformOrigin' , canvas, '0% 0%');
      if (textLayerDiv) {
        CustomStyle.setProp('transform' , textLayerDiv, cssScale);
        CustomStyle.setProp('transformOrigin' , textLayerDiv, '0% 0%');
      }
    }

    var ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    ctx._scaleX = outputScale.sx;
    ctx._scaleY = outputScale.sy;
    if (outputScale.scaled) {
      ctx.scale(outputScale.sx, outputScale.sy);
    }
    
    var checkIfDocumentFontsUsed = !PDFView.pdfDocument.embeddedFontsUsed;

    

    var self = this;
    var renderingWasReset = false;
    function pageViewDrawCallback(error) {
      if (renderingWasReset) {
        return;
      }

      self.renderingState = RenderingStates.FINISHED;

      if (self.loadingIconDiv) {
        div.removeChild(self.loadingIconDiv);
        delete self.loadingIconDiv;
      }

      if (checkIfDocumentFontsUsed && PDFView.pdfDocument.embeddedFontsUsed &&
          !PDFView.supportsDocumentFonts) {
        console.error(mozL10n.get('web_fonts_disabled', null,
          'Web fonts are disabled: unable to use embedded PDF fonts.'));
        PDFView.fallback();
      }
      if (error) {
        PDFView.error(mozL10n.get('rendering_error', null,
          'An error occurred while rendering the page.'), error);
      }

      self.stats = pdfPage.stats;
      self.updateStats();
      if (self.onAfterDraw)
        self.onAfterDraw();

      cache.push(self);
      callback();
    }

    var renderContext = {
      canvasContext: ctx,
      viewport: this.viewport,
      textLayer: textLayer,
      continueCallback: function pdfViewcContinueCallback(cont) {
        if (self.renderingState === RenderingStates.INITIAL) {
          
          renderingWasReset = true;
          return;
        }

        if (PDFView.highestPriorityPage !== 'page' + self.id) {
          self.renderingState = RenderingStates.PAUSED;
          self.resume = function resumeCallback() {
            self.renderingState = RenderingStates.RUNNING;
            cont();
          };
          return;
        }
        cont();
      }
    };
    this.pdfPage.render(renderContext).then(
      function pdfPageRenderCallback() {
        pageViewDrawCallback(null);
      },
      function pdfPageRenderError(error) {
        pageViewDrawCallback(error);
      }
    );

    if (textLayer) {
      this.getTextContent().then(
        function textContentResolved(textContent) {
          textLayer.setTextContent(textContent);
        }
      );
    }

    setupAnnotations(this.pdfPage, this.viewport);
    div.setAttribute('data-loaded', true);
  };

  this.beforePrint = function pageViewBeforePrint() {
    var pdfPage = this.pdfPage;
    var viewport = pdfPage.getViewport(1);
    
    
    var PRINT_OUTPUT_SCALE = 2;
    var canvas = this.canvas = document.createElement('canvas');
    canvas.width = Math.floor(viewport.width) * PRINT_OUTPUT_SCALE;
    canvas.height = Math.floor(viewport.height) * PRINT_OUTPUT_SCALE;
    canvas.style.width = (PRINT_OUTPUT_SCALE * viewport.width) + 'pt';
    canvas.style.height = (PRINT_OUTPUT_SCALE * viewport.height) + 'pt';
    var cssScale = 'scale(' + (1 / PRINT_OUTPUT_SCALE) + ', ' +
                              (1 / PRINT_OUTPUT_SCALE) + ')';
    CustomStyle.setProp('transform' , canvas, cssScale);
    CustomStyle.setProp('transformOrigin' , canvas, '0% 0%');

    var printContainer = document.getElementById('printContainer');
    printContainer.appendChild(canvas);

    var self = this;
    canvas.mozPrintCallback = function(obj) {
      var ctx = obj.context;

      ctx.save();
      ctx.fillStyle = 'rgb(255, 255, 255)';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.restore();
      ctx.scale(PRINT_OUTPUT_SCALE, PRINT_OUTPUT_SCALE);

      var renderContext = {
        canvasContext: ctx,
        viewport: viewport
      };

      pdfPage.render(renderContext).then(function() {
        
        obj.done();
        self.pdfPage.destroy();
      }, function(error) {
        console.error(error);
        
        
        if ('abort' in obj)
          obj.abort();
        else
          obj.done();
        self.pdfPage.destroy();
      });
    };
  };

  this.updateStats = function pageViewUpdateStats() {
    if (PDFJS.pdfBug && Stats.enabled) {
      var stats = this.stats;
      Stats.add(this.id, stats);
    }
  };
};

var ThumbnailView = function thumbnailView(container, pdfPage, id) {
  var anchor = document.createElement('a');
  anchor.href = PDFView.getAnchorUrl('#page=' + id);
  anchor.title = mozL10n.get('thumb_page_title', {page: id}, 'Page {{page}}');
  anchor.onclick = function stopNavigation() {
    PDFView.page = id;
    return false;
  };

  var rotation = 0;
  var totalRotation = (rotation + pdfPage.rotate) % 360;
  var viewport = pdfPage.getViewport(1, totalRotation);
  var pageWidth = this.width = viewport.width;
  var pageHeight = this.height = viewport.height;
  var pageRatio = pageWidth / pageHeight;
  this.id = id;

  var canvasWidth = 98;
  var canvasHeight = canvasWidth / this.width * this.height;
  var scaleX = this.scaleX = (canvasWidth / pageWidth);
  var scaleY = this.scaleY = (canvasHeight / pageHeight);

  var div = this.el = document.createElement('div');
  div.id = 'thumbnailContainer' + id;
  div.className = 'thumbnail';

  if (id === 1) {
    
    
    div.classList.add('selected');
  }

  var ring = document.createElement('div');
  ring.className = 'thumbnailSelectionRing';
  ring.style.width = canvasWidth + 'px';
  ring.style.height = canvasHeight + 'px';

  div.appendChild(ring);
  anchor.appendChild(div);
  container.appendChild(anchor);

  this.hasImage = false;
  this.renderingState = RenderingStates.INITIAL;

  this.updateRotation = function(rot) {

    rotation = rot;
    totalRotation = (rotation + pdfPage.rotate) % 360;
    viewport = pdfPage.getViewport(1, totalRotation);
    pageWidth = this.width = viewport.width;
    pageHeight = this.height = viewport.height;
    pageRatio = pageWidth / pageHeight;

    canvasHeight = canvasWidth / this.width * this.height;
    scaleX = this.scaleX = (canvasWidth / pageWidth);
    scaleY = this.scaleY = (canvasHeight / pageHeight);

    div.removeAttribute('data-loaded');
    ring.textContent = '';
    ring.style.width = canvasWidth + 'px';
    ring.style.height = canvasHeight + 'px';

    this.hasImage = false;
    this.renderingState = RenderingStates.INITIAL;
    this.resume = null;
  };

  function getPageDrawContext() {
    var canvas = document.createElement('canvas');
    canvas.id = 'thumbnail' + id;

    canvas.width = canvasWidth;
    canvas.height = canvasHeight;
    canvas.className = 'thumbnailImage';
    canvas.setAttribute('aria-label', mozL10n.get('thumb_page_canvas',
      {page: id}, 'Thumbnail of Page {{page}}'));

    div.setAttribute('data-loaded', true);

    ring.appendChild(canvas);

    var ctx = canvas.getContext('2d');
    ctx.save();
    ctx.fillStyle = 'rgb(255, 255, 255)';
    ctx.fillRect(0, 0, canvasWidth, canvasHeight);
    ctx.restore();
    return ctx;
  }

  this.drawingRequired = function thumbnailViewDrawingRequired() {
    return !this.hasImage;
  };

  this.draw = function thumbnailViewDraw(callback) {
    if (this.renderingState !== RenderingStates.INITIAL) {
      console.error('Must be in new state before drawing');
    }

    this.renderingState = RenderingStates.RUNNING;
    if (this.hasImage) {
      callback();
      return;
    }

    var self = this;
    var ctx = getPageDrawContext();
    var drawViewport = pdfPage.getViewport(scaleX, totalRotation);
    var renderContext = {
      canvasContext: ctx,
      viewport: drawViewport,
      continueCallback: function(cont) {
        if (PDFView.highestPriorityPage !== 'thumbnail' + self.id) {
          self.renderingState = RenderingStates.PAUSED;
          self.resume = function() {
            self.renderingState = RenderingStates.RUNNING;
            cont();
          };
          return;
        }
        cont();
      }
    };
    pdfPage.render(renderContext).then(
      function pdfPageRenderCallback() {
        self.renderingState = RenderingStates.FINISHED;
        callback();
      },
      function pdfPageRenderError(error) {
        self.renderingState = RenderingStates.FINISHED;
        callback();
      }
    );
    this.hasImage = true;
  };

  this.setImage = function thumbnailViewSetImage(img) {
    if (this.hasImage || !img)
      return;
    this.renderingState = RenderingStates.FINISHED;
    var ctx = getPageDrawContext();
    ctx.drawImage(img, 0, 0, img.width, img.height,
                  0, 0, ctx.canvas.width, ctx.canvas.height);

    this.hasImage = true;
  };
};

var DocumentOutlineView = function documentOutlineView(outline) {
  var outlineView = document.getElementById('outlineView');
  while (outlineView.firstChild)
    outlineView.removeChild(outlineView.firstChild);

  function bindItemLink(domObj, item) {
    domObj.href = PDFView.getDestinationHash(item.dest);
    domObj.onclick = function documentOutlineViewOnclick(e) {
      PDFView.navigateTo(item.dest);
      return false;
    };
  }

  if (!outline) {
    var noOutline = document.createElement('div');
    noOutline.classList.add('noOutline');
    noOutline.textContent = mozL10n.get('no_outline', null,
      'No Outline Available');
    outlineView.appendChild(noOutline);
    return;
  }

  var queue = [{parent: outlineView, items: outline}];
  while (queue.length > 0) {
    var levelData = queue.shift();
    var i, n = levelData.items.length;
    for (i = 0; i < n; i++) {
      var item = levelData.items[i];
      var div = document.createElement('div');
      div.className = 'outlineItem';
      var a = document.createElement('a');
      bindItemLink(a, item);
      a.textContent = item.title;
      div.appendChild(a);

      if (item.items.length > 0) {
        var itemsDiv = document.createElement('div');
        itemsDiv.className = 'outlineItems';
        div.appendChild(itemsDiv);
        queue.push({parent: itemsDiv, items: item.items});
      }

      levelData.parent.appendChild(div);
    }
  }
};


var CustomStyle = (function CustomStyleClosure() {

  
  
  
  
  var prefixes = ['ms', 'Moz', 'Webkit', 'O'];
  var _cache = { };

  function CustomStyle() {
  }

  CustomStyle.getProp = function get(propName, element) {
    
    if (arguments.length == 1 && typeof _cache[propName] == 'string') {
      return _cache[propName];
    }

    element = element || document.documentElement;
    var style = element.style, prefixed, uPropName;

    
    if (typeof style[propName] == 'string') {
      return (_cache[propName] = propName);
    }

    
    uPropName = propName.charAt(0).toUpperCase() + propName.slice(1);

    
    for (var i = 0, l = prefixes.length; i < l; i++) {
      prefixed = prefixes[i] + uPropName;
      if (typeof style[prefixed] == 'string') {
        return (_cache[propName] = prefixed);
      }
    }

    
    return (_cache[propName] = 'undefined');
  };

  CustomStyle.setProp = function set(propName, element, str) {
    var prop = this.getProp(propName);
    if (prop != 'undefined')
      element.style[prop] = str;
  };

  return CustomStyle;
})();

var TextLayerBuilder = function textLayerBuilder(textLayerDiv, pageIdx) {
  var textLayerFrag = document.createDocumentFragment();

  this.textLayerDiv = textLayerDiv;
  this.layoutDone = false;
  this.divContentDone = false;
  this.pageIdx = pageIdx;
  this.matches = [];

  this.beginLayout = function textLayerBuilderBeginLayout() {
    this.textDivs = [];
    this.textLayerQueue = [];
    this.renderingDone = false;
  };

  this.endLayout = function textLayerBuilderEndLayout() {
    this.layoutDone = true;
    this.insertDivContent();
  };

  this.renderLayer = function textLayerBuilderRenderLayer() {
    var self = this;
    var textDivs = this.textDivs;
    var bidiTexts = this.textContent.bidiTexts;
    var textLayerDiv = this.textLayerDiv;
    var canvas = document.createElement('canvas');
    var ctx = canvas.getContext('2d');

    
    
    var MAX_TEXT_DIVS_TO_RENDER = 100000;
    if (textDivs.length > MAX_TEXT_DIVS_TO_RENDER)
      return;

    for (var i = 0, ii = textDivs.length; i < ii; i++) {
      var textDiv = textDivs[i];
      if ('isWhitespace' in textDiv.dataset) {
        continue;
      }
      textLayerFrag.appendChild(textDiv);

      ctx.font = textDiv.style.fontSize + ' ' + textDiv.style.fontFamily;
      var width = ctx.measureText(textDiv.textContent).width;

      if (width > 0) {
        var textScale = textDiv.dataset.canvasWidth / width;

        var transform = 'scale(' + textScale + ', 1)';
        if (bidiTexts[i].dir === 'ttb') {
          transform = 'rotate(90deg) ' + transform;
        }
        CustomStyle.setProp('transform' , textDiv, transform);
        CustomStyle.setProp('transformOrigin' , textDiv, '0% 0%');

        textLayerDiv.appendChild(textDiv);
      }
    }

    this.renderingDone = true;
    this.updateMatches();

    textLayerDiv.appendChild(textLayerFrag);
  };

  this.setupRenderLayoutTimer = function textLayerSetupRenderLayoutTimer() {
    
    
    var RENDER_DELAY = 200; 
    var self = this;
    if (Date.now() - PDFView.lastScroll > RENDER_DELAY) {
      
      this.renderLayer();
    } else {
      
      if (this.renderTimer)
        clearTimeout(this.renderTimer);
      this.renderTimer = setTimeout(function() {
        self.setupRenderLayoutTimer();
      }, RENDER_DELAY);
    }
  };

  this.appendText = function textLayerBuilderAppendText(geom) {
    var textDiv = document.createElement('div');

    
    var fontHeight = geom.fontSize * Math.abs(geom.vScale);
    textDiv.dataset.canvasWidth = geom.canvasWidth * geom.hScale;
    textDiv.dataset.fontName = geom.fontName;

    textDiv.style.fontSize = fontHeight + 'px';
    textDiv.style.fontFamily = geom.fontFamily;
    textDiv.style.left = geom.x + 'px';
    textDiv.style.top = (geom.y - fontHeight) + 'px';

    

    this.textDivs.push(textDiv);
  };

  this.insertDivContent = function textLayerUpdateTextContent() {
    
    
    if (!this.layoutDone || this.divContentDone || !this.textContent)
      return;

    this.divContentDone = true;

    var textDivs = this.textDivs;
    var bidiTexts = this.textContent.bidiTexts;

    for (var i = 0; i < bidiTexts.length; i++) {
      var bidiText = bidiTexts[i];
      var textDiv = textDivs[i];
      if (!/\S/.test(bidiText.str)) {
        textDiv.dataset.isWhitespace = true;
        continue;
      }

      textDiv.textContent = bidiText.str;
      
      textDiv.dir = bidiText.dir === 'rtl' ? 'rtl' : 'ltr';
    }

    this.setupRenderLayoutTimer();
  };

  this.setTextContent = function textLayerBuilderSetTextContent(textContent) {
    this.textContent = textContent;
    this.insertDivContent();
  };

  this.convertMatches = function textLayerBuilderConvertMatches(matches) {
    var i = 0;
    var iIndex = 0;
    var bidiTexts = this.textContent.bidiTexts;
    var end = bidiTexts.length - 1;
    var queryLen = PDFFindController.state.query.length;

    var lastDivIdx = -1;
    var pos;

    var ret = [];

    
    for (var m = 0; m < matches.length; m++) {
      var matchIdx = matches[m];
      

      
      while (i !== end && matchIdx >= (iIndex + bidiTexts[i].str.length)) {
        iIndex += bidiTexts[i].str.length;
        i++;
      }

      
      if (i == bidiTexts.length) {
        console.error('Could not find matching mapping');
      }

      var match = {
        begin: {
          divIdx: i,
          offset: matchIdx - iIndex
        }
      };

      
      matchIdx += queryLen;

      
      
      while (i !== end && matchIdx > (iIndex + bidiTexts[i].str.length)) {
        iIndex += bidiTexts[i].str.length;
        i++;
      }

      match.end = {
        divIdx: i,
        offset: matchIdx - iIndex
      };
      ret.push(match);
    }

    return ret;
  };

  this.renderMatches = function textLayerBuilder_renderMatches(matches) {
    
    if (matches.length === 0) {
      return;
    }

    var bidiTexts = this.textContent.bidiTexts;
    var textDivs = this.textDivs;
    var prevEnd = null;
    var isSelectedPage = this.pageIdx === PDFFindController.selected.pageIdx;
    var selectedMatchIdx = PDFFindController.selected.matchIdx;
    var highlightAll = PDFFindController.state.highlightAll;

    var infty = {
      divIdx: -1,
      offset: undefined
    };

    function beginText(begin, className) {
      var divIdx = begin.divIdx;
      var div = textDivs[divIdx];
      div.textContent = '';

      var content = bidiTexts[divIdx].str.substring(0, begin.offset);
      var node = document.createTextNode(content);
      if (className) {
        var isSelected = isSelectedPage &&
                          divIdx === selectedMatchIdx;
        var span = document.createElement('span');
        span.className = className + (isSelected ? ' selected' : '');
        span.appendChild(node);
        div.appendChild(span);
        return;
      }
      div.appendChild(node);
    }

    function appendText(from, to, className) {
      var divIdx = from.divIdx;
      var div = textDivs[divIdx];

      var content = bidiTexts[divIdx].str.substring(from.offset, to.offset);
      var node = document.createTextNode(content);
      if (className) {
        var span = document.createElement('span');
        span.className = className;
        span.appendChild(node);
        div.appendChild(span);
        return;
      }
      div.appendChild(node);
    }

    function highlightDiv(divIdx, className) {
      textDivs[divIdx].className = className;
    }

    var i0 = selectedMatchIdx, i1 = i0 + 1, i;

    if (highlightAll) {
      i0 = 0;
      i1 = matches.length;
    } else if (!isSelectedPage) {
      
      return;
    }

    for (i = i0; i < i1; i++) {
      var match = matches[i];
      var begin = match.begin;
      var end = match.end;

      var isSelected = isSelectedPage && i === selectedMatchIdx;
      var highlightSuffix = (isSelected ? ' selected' : '');
      if (isSelected)
        scrollIntoView(textDivs[begin.divIdx], {top: -50});

      
      if (!prevEnd || begin.divIdx !== prevEnd.divIdx) {
        
        if (prevEnd !== null) {
          appendText(prevEnd, infty);
        }
        
        beginText(begin);
      } else {
        appendText(prevEnd, begin);
      }

      if (begin.divIdx === end.divIdx) {
        appendText(begin, end, 'highlight' + highlightSuffix);
      } else {
        appendText(begin, infty, 'highlight begin' + highlightSuffix);
        for (var n = begin.divIdx + 1; n < end.divIdx; n++) {
          highlightDiv(n, 'highlight middle' + highlightSuffix);
        }
        beginText(end, 'highlight end' + highlightSuffix);
      }
      prevEnd = end;
    }

    if (prevEnd) {
      appendText(prevEnd, infty);
    }
  };

  this.updateMatches = function textLayerUpdateMatches() {
    
    if (!this.renderingDone)
      return;

    
    var matches = this.matches;
    var textDivs = this.textDivs;
    var bidiTexts = this.textContent.bidiTexts;
    var clearedUntilDivIdx = -1;

    
    for (var i = 0; i < matches.length; i++) {
      var match = matches[i];
      var begin = Math.max(clearedUntilDivIdx, match.begin.divIdx);
      for (var n = begin; n <= match.end.divIdx; n++) {
        var div = textDivs[n];
        div.textContent = bidiTexts[n].str;
        div.className = '';
      }
      clearedUntilDivIdx = match.end.divIdx + 1;
    }

    if (!PDFFindController.active)
      return;

    
    
    this.matches = matches =
      this.convertMatches(PDFFindController.pageMatches[this.pageIdx] || []);

    this.renderMatches(this.matches);
  };
};

document.addEventListener('DOMContentLoaded', function webViewerLoad(evt) {
  PDFView.initialize();
  var params = PDFView.parseQueryString(document.location.search.substring(1));

  var file = window.location.toString()

  document.getElementById('openFile').setAttribute('hidden', 'true');

  
  var hash = document.location.hash.substring(1);
  var hashParams = PDFView.parseQueryString(hash);

  if ('disableWorker' in hashParams)
    PDFJS.disableWorker = (hashParams['disableWorker'] === 'true');


  if ('textLayer' in hashParams) {
    switch (hashParams['textLayer']) {
      case 'off':
        PDFJS.disableTextLayer = true;
        break;
      case 'visible':
      case 'shadow':
      case 'hover':
        var viewer = document.getElementById('viewer');
        viewer.classList.add('textLayer-' + hashParams['textLayer']);
        break;
    }
  }

  if ('pdfBug' in hashParams && FirefoxCom.requestSync('pdfBugEnabled')) {
    PDFJS.pdfBug = true;
    var pdfBug = hashParams['pdfBug'];
    var enabled = pdfBug.split(',');
    PDFBug.enable(enabled);
    PDFBug.init();
  }

  if (!PDFView.supportsPrinting) {
    document.getElementById('print').classList.add('hidden');
  }

  if (!PDFView.supportsFullscreen) {
    document.getElementById('fullscreen').classList.add('hidden');
  }

  if (PDFView.supportsIntegratedFind) {
    document.querySelector('#viewFind').classList.add('hidden');
  }

  
  
  PDFJS.LogManager.addLogger({
    warn: function() {
      PDFView.fallback();
    }
  });

  var mainContainer = document.getElementById('mainContainer');
  var outerContainer = document.getElementById('outerContainer');
  mainContainer.addEventListener('transitionend', function(e) {
    if (e.target == mainContainer) {
      var event = document.createEvent('UIEvents');
      event.initUIEvent('resize', false, false, window, 0);
      window.dispatchEvent(event);
      outerContainer.classList.remove('sidebarMoving');
    }
  }, true);

  document.getElementById('sidebarToggle').addEventListener('click',
    function() {
      this.classList.toggle('toggled');
      outerContainer.classList.add('sidebarMoving');
      outerContainer.classList.toggle('sidebarOpen');
      PDFView.sidebarOpen = outerContainer.classList.contains('sidebarOpen');
      PDFView.renderHighestPriority();
    });

  document.getElementById('viewThumbnail').addEventListener('click',
    function() {
      PDFView.switchSidebarView('thumbs');
    });

  document.getElementById('viewOutline').addEventListener('click',
    function() {
      PDFView.switchSidebarView('outline');
    });

  document.getElementById('previous').addEventListener('click',
    function() {
      PDFView.page--;
    });

  document.getElementById('next').addEventListener('click',
    function() {
      PDFView.page++;
    });

  document.querySelector('.zoomIn').addEventListener('click',
    function() {
      PDFView.zoomIn();
    });

  document.querySelector('.zoomOut').addEventListener('click',
    function() {
      PDFView.zoomOut();
    });

  document.getElementById('fullscreen').addEventListener('click',
    function() {
      PDFView.fullscreen();
    });

  document.getElementById('openFile').addEventListener('click',
    function() {
      document.getElementById('fileInput').click();
    });

  document.getElementById('print').addEventListener('click',
    function() {
      window.print();
    });

  document.getElementById('download').addEventListener('click',
    function() {
      PDFView.download();
    });

  document.getElementById('pageNumber').addEventListener('click',
    function() {
      this.select();
    });

  document.getElementById('pageNumber').addEventListener('change',
    function() {
      
      PDFView.page = (this.value | 0);

      if (this.value !== (this.value | 0).toString()) {
        this.value = PDFView.page;
      }
    });

  document.getElementById('scaleSelect').addEventListener('change',
    function() {
      PDFView.parseScale(this.value);
    });

  document.getElementById('first_page').addEventListener('click',
    function() {
      PDFView.page = 1;
    });

  document.getElementById('last_page').addEventListener('click',
    function() {
      PDFView.page = PDFView.pdfDocument.numPages;
    });

  document.getElementById('page_rotate_ccw').addEventListener('click',
    function() {
      PDFView.rotatePages(-90);
    });

  document.getElementById('page_rotate_cw').addEventListener('click',
    function() {
      PDFView.rotatePages(90);
    });

  if (FirefoxCom.requestSync('getLoadingType') == 'passive') {
    PDFView.setTitleUsingUrl(file);
    PDFView.initPassiveLoading();
    return;
  }

  PDFView.open(file, 0);
}, true);

function updateViewarea() {

  if (!PDFView.initialized)
    return;
  var visible = PDFView.getVisiblePages();
  var visiblePages = visible.views;
  if (visiblePages.length === 0) {
    return;
  }

  PDFView.renderHighestPriority();

  var currentId = PDFView.page;
  var firstPage = visible.first;

  for (var i = 0, ii = visiblePages.length, stillFullyVisible = false;
       i < ii; ++i) {
    var page = visiblePages[i];

    if (page.percent < 100)
      break;

    if (page.id === PDFView.page) {
      stillFullyVisible = true;
      break;
    }
  }

  if (!stillFullyVisible) {
    currentId = visiblePages[0].id;
  }

  if (!PDFView.isFullscreen) {
    updateViewarea.inProgress = true; 
    PDFView.page = currentId;
    updateViewarea.inProgress = false;
  }

  var currentScale = PDFView.currentScale;
  var currentScaleValue = PDFView.currentScaleValue;
  var normalizedScaleValue = currentScaleValue == currentScale ?
    currentScale * 100 : currentScaleValue;

  var pageNumber = firstPage.id;
  var pdfOpenParams = '#page=' + pageNumber;
  pdfOpenParams += '&zoom=' + normalizedScaleValue;
  var currentPage = PDFView.pages[pageNumber - 1];
  var topLeft = currentPage.getPagePoint(PDFView.container.scrollLeft,
    (PDFView.container.scrollTop - firstPage.y));
  pdfOpenParams += ',' + Math.round(topLeft[0]) + ',' + Math.round(topLeft[1]);

  var store = PDFView.store;
  store.initializedPromise.then(function() {
    store.set('exists', true);
    store.set('page', pageNumber);
    store.set('zoom', normalizedScaleValue);
    store.set('scrollLeft', Math.round(topLeft[0]));
    store.set('scrollTop', Math.round(topLeft[1]));
  });
  var href = PDFView.getAnchorUrl(pdfOpenParams);
  document.getElementById('viewBookmark').href = href;
}

window.addEventListener('resize', function webViewerResize(evt) {
  if (PDFView.initialized &&
      (document.getElementById('pageWidthOption').selected ||
      document.getElementById('pageFitOption').selected ||
      document.getElementById('pageAutoOption').selected))
      PDFView.parseScale(document.getElementById('scaleSelect').value);
  updateViewarea();
});

window.addEventListener('hashchange', function webViewerHashchange(evt) {
  PDFView.setHash(document.location.hash.substring(1));
});

window.addEventListener('change', function webViewerChange(evt) {
  var files = evt.target.files;
  if (!files || files.length === 0)
    return;

  
  var fileReader = new FileReader();
  fileReader.onload = function webViewerChangeFileReaderOnload(evt) {
    var buffer = evt.target.result;
    var uint8Array = new Uint8Array(buffer);
    PDFView.open(uint8Array, 0);
  };

  var file = files[0];
  fileReader.readAsArrayBuffer(file);
  PDFView.setTitleUsingUrl(file.name);

  
  document.getElementById('viewBookmark').setAttribute('hidden', 'true');
  document.getElementById('download').setAttribute('hidden', 'true');
}, true);

function selectScaleOption(value) {
  var options = document.getElementById('scaleSelect').options;
  var predefinedValueFound = false;
  for (var i = 0; i < options.length; i++) {
    var option = options[i];
    if (option.value != value) {
      option.selected = false;
      continue;
    }
    option.selected = true;
    predefinedValueFound = true;
  }
  return predefinedValueFound;
}

window.addEventListener('localized', function localized(evt) {
  document.getElementsByTagName('html')[0].dir = mozL10n.getDirection();

  
  PDFView.animationStartedPromise.then(
    function() {
      var container = document.getElementById('scaleSelectContainer');
      var select = document.getElementById('scaleSelect');
      select.setAttribute('style', 'min-width: inherit;');
      var width = select.clientWidth + 8;
      select.setAttribute('style', 'min-width: ' + (width + 20) + 'px;');
      container.setAttribute('style', 'min-width: ' + width + 'px; ' +
                                      'max-width: ' + width + 'px;');
  });
}, true);

window.addEventListener('scalechange', function scalechange(evt) {
  var customScaleOption = document.getElementById('customScaleOption');
  customScaleOption.selected = false;

  if (!evt.resetAutoSettings &&
       (document.getElementById('pageWidthOption').selected ||
        document.getElementById('pageFitOption').selected ||
        document.getElementById('pageAutoOption').selected)) {
      updateViewarea();
      return;
  }

  var predefinedValueFound = selectScaleOption('' + evt.scale);
  if (!predefinedValueFound) {
    customScaleOption.textContent = Math.round(evt.scale * 10000) / 100 + '%';
    customScaleOption.selected = true;
  }
  
  document.getElementById('zoom_out').disabled = (evt.scale === MIN_SCALE);
  document.getElementById('zoom_in').disabled = (evt.scale === MAX_SCALE);

  updateViewarea();
}, true);

window.addEventListener('pagechange', function pagechange(evt) {
  var page = evt.pageNumber;
  if (PDFView.previousPageNumber !== page) {
    document.getElementById('pageNumber').value = page;
    var selected = document.querySelector('.thumbnail.selected');
    if (selected)
      selected.classList.remove('selected');
    var thumbnail = document.getElementById('thumbnailContainer' + page);
    thumbnail.classList.add('selected');
    var visibleThumbs = PDFView.getVisibleThumbs();
    var numVisibleThumbs = visibleThumbs.views.length;
    
    if (numVisibleThumbs > 0) {
      var first = visibleThumbs.first.id;
      
      var last = numVisibleThumbs > 1 ?
                  visibleThumbs.last.id : first;
      if (page <= first || page >= last)
        scrollIntoView(thumbnail);
    }

  }
  document.getElementById('previous').disabled = (page <= 1);
  document.getElementById('next').disabled = (page >= PDFView.pages.length);
}, true);


window.addEventListener('DOMMouseScroll', function(evt) {
  if (evt.ctrlKey) {
    evt.preventDefault();

    var ticks = evt.detail;
    var direction = (ticks > 0) ? 'zoomOut' : 'zoomIn';
    for (var i = 0, length = Math.abs(ticks); i < length; i++)
      PDFView[direction]();
  } else if (PDFView.isFullscreen) {
    var FIREFOX_DELTA_FACTOR = -40;
    PDFView.mouseScroll(evt.detail * FIREFOX_DELTA_FACTOR);
  }
}, false);

window.addEventListener('mousemove', function mousemove(evt) {
  if (PDFView.isFullscreen) {
    PDFView.showPresentationControls();
  }
}, false);

window.addEventListener('mousedown', function mousedown(evt) {
  if (PDFView.isFullscreen && evt.button === 0) {
    
    
    var targetHref = evt.target.href;
    var internalLink = targetHref && (targetHref.replace(/#.*$/, '') ===
                                      window.location.href.replace(/#.*$/, ''));
    if (!internalLink) {
      
      evt.preventDefault();
      PDFView.page++;
    }
  }
}, false);

window.addEventListener('click', function click(evt) {
  if (PDFView.isFullscreen && evt.button === 0) {
    
    
    evt.preventDefault();
  }
}, false);

window.addEventListener('keydown', function keydown(evt) {
  var handled = false;
  var cmd = (evt.ctrlKey ? 1 : 0) |
            (evt.altKey ? 2 : 0) |
            (evt.shiftKey ? 4 : 0) |
            (evt.metaKey ? 8 : 0);

  
  
  if (cmd === 1 || cmd === 8 || cmd === 5 || cmd === 12) {
    
    switch (evt.keyCode) {
      case 70:
        if (!PDFView.supportsIntegratedFind) {
          PDFFindBar.toggle();
          handled = true;
        }
        break;
      case 61: 
      case 107: 
      case 187: 
      case 171: 
        PDFView.zoomIn();
        handled = true;
        break;
      case 173: 
      case 109: 
      case 189: 
        PDFView.zoomOut();
        handled = true;
        break;
      case 48: 
      case 96: 
        PDFView.parseScale(DEFAULT_SCALE, true);
        handled = false; 
        break;
    }
  }

  
  if (cmd == 1 || cmd == 8 || cmd == 5 || cmd == 12) {
    switch (evt.keyCode) {
      case 71: 
        if (!PDFView.supportsIntegratedFind) {
          PDFFindBar.dispatchEvent('again', cmd == 5 || cmd == 12);
          handled = true;
        }
        break;
    }
  }

  if (handled) {
    evt.preventDefault();
    return;
  }

  
  
  var curElement = document.activeElement;
  if (curElement && (curElement.tagName == 'INPUT' ||
                     curElement.tagName == 'SELECT')) {
    return;
  }
  var controlsElement = document.getElementById('toolbar');
  while (curElement) {
    if (curElement === controlsElement && !PDFView.isFullscreen)
      return; 
    curElement = curElement.parentNode;
  }

  if (cmd === 0) { 
    switch (evt.keyCode) {
      case 38: 
      case 33: 
      case 8: 
        if (!PDFView.isFullscreen && PDFView.currentScaleValue !== 'page-fit') {
          break;
        }
        
        
      case 37: 
        
        if (PDFView.isHorizontalScrollbarEnabled) {
          break;
        }
        
      case 75: 
      case 80: 
        PDFView.page--;
        handled = true;
        break;
      case 27: 
        if (!PDFView.supportsIntegratedFind && PDFFindBar.opened) {
          PDFFindBar.close();
          handled = true;
        }
        break;
      case 40: 
      case 34: 
      case 32: 
        if (!PDFView.isFullscreen && PDFView.currentScaleValue !== 'page-fit') {
          break;
        }
        
      case 39: 
        
        if (PDFView.isHorizontalScrollbarEnabled) {
          break;
        }
        
      case 74: 
      case 78: 
        PDFView.page++;
        handled = true;
        break;

      case 36: 
        if (PDFView.isFullscreen) {
          PDFView.page = 1;
          handled = true;
        }
        break;
      case 35: 
        if (PDFView.isFullscreen) {
          PDFView.page = PDFView.pdfDocument.numPages;
          handled = true;
        }
        break;

      case 82: 
        PDFView.rotatePages(90);
        break;
    }
  }

  if (cmd == 4) { 
    switch (evt.keyCode) {
      case 82: 
        PDFView.rotatePages(-90);
        break;
    }
  }

  if (handled) {
    evt.preventDefault();
    PDFView.clearMouseScrollState();
  }
});

window.addEventListener('beforeprint', function beforePrint(evt) {
  PDFView.beforePrint();
});

window.addEventListener('afterprint', function afterPrint(evt) {
  PDFView.afterPrint();
});

(function fullscreenClosure() {
  function fullscreenChange(e) {
    var isFullscreen = document.fullscreenElement || document.mozFullScreen ||
        document.webkitIsFullScreen;

    if (!isFullscreen) {
      PDFView.exitFullscreen();
    }
  }

  window.addEventListener('fullscreenchange', fullscreenChange, false);
  window.addEventListener('mozfullscreenchange', fullscreenChange, false);
  window.addEventListener('webkitfullscreenchange', fullscreenChange, false);
})();

(function animationStartedClosure() {
  
  
  var requestAnimationFrame = window.requestAnimationFrame ||
                              window.mozRequestAnimationFrame ||
                              window.webkitRequestAnimationFrame ||
                              window.oRequestAnimationFrame ||
                              window.msRequestAnimationFrame ||
                              function startAtOnce(callback) { callback(); };
  PDFView.animationStartedPromise = new PDFJS.Promise();
  requestAnimationFrame(function onAnimationFrame() {
    PDFView.animationStartedPromise.resolve();
  });
})();


