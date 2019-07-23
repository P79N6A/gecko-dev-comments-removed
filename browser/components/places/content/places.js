






































const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";







function selectPlaceURI(placeURI) {
  PlacesOrganizer._places.selectPlaceURI(placeURI);
}

var PlacesOrganizer = {
  _places: null,
  _content: null,

  init: function PO_init() {
    var self = this;
    
    
    setTimeout(function() { self._init(); }, 0);
  },
  
  _init: function PO__init() {
    this._places = document.getElementById("placesList");
    this._content = document.getElementById("placeContent");

    OptionsFilter.init(Groupers);
    Groupers.init();

    
    var placeURI = "place:";
    if ("arguments" in window)
      placeURI = window.arguments[0];

    selectPlaceURI(placeURI);

    var view = this._content.treeBoxObject.view;
    if (view.rowCount > 0)
      view.selection.select(0);

    this._content.focus();

    
    PlacesSearchBox.init();

    
    PlacesQueryBuilder.init();
  },

  destroy: function PO_destroy() {
    OptionsFilter.destroy();
  },

  HEADER_TYPE_SHOWING: 1,
  HEADER_TYPE_SEARCH: 2,
  HEADER_TYPE_ADVANCED_SEARCH: 3,

  








  setHeaderText: function PO_setHeaderText(type, text) {
    NS_ASSERT(type == 1 || type == 2 || type == 3, "Invalid Header Type");
    var prefix = document.getElementById("showingPrefix");
    prefix.setAttribute("value",
                        PlacesUtils.getString("headerTextPrefix" + type));

    var contentTitle = document.getElementById("contentTitle");
    contentTitle.setAttribute("value", text);
    
    
    var searchModifiers = document.getElementById("searchModifiers");
    searchModifiers.hidden = type == this.HEADER_TYPE_SHOWING;
  },

  onPlaceURIKeypress: function PO_onPlaceURIKeypress(aEvent) {
    var keyCode = aEvent.keyCode;
    if (keyCode == KeyEvent.DOM_VK_RETURN)
      this.loadPlaceURI();
    else if (keyCode == KeyEvent.DOM_VK_ESCAPE) {
      event.target.value = "";
      this.onPlaceSelected(true);
    }
  },

  


  toggleDebugPanel: function PO_toggleDebugPanel() {
    var dp = document.getElementById("debugPanel");
    dp.hidden = !dp.hidden;
    if (!dp.hidden)
      document.getElementById("placeURI").focus();
  },

  


  loadPlaceURI: function PO_loadPlaceURI() {
    var placeURI = document.getElementById("placeURI");
    var queriesRef = { }, optionsRef = { };
    PlacesUtils.history.queryStringToQueries(placeURI.value, 
                                             queriesRef, { }, optionsRef);
    
    var autoFilterResults = document.getElementById("autoFilterResults");
    if (autoFilterResults.checked) {
      var options = 
        OptionsFilter.filter(queriesRef.value, optionsRef.value, null);
    }
    else
      options = optionsRef.value;
    this._content.load(queriesRef.value, options);

    this.setHeaderText(this.HEADER_TYPE_SHOWING, "Debug results for: " + placeURI.value);

    this.updateLoadedURI();

    placeURI.focus();
  },

  


  updateLoadedURI: function PO_updateLoadedURI() {
    var queryNode = asQuery(this._content.getResult().root);
    var queries = queryNode.getQueries({});
    var options = queryNode.queryOptions;
    var loadedURI = document.getElementById("loadedURI");
    loadedURI.value =
      PlacesUtils.history.queriesToQueryString(queries, queries.length, 
                                               options);
  },

  





  onPlaceSelected: function PO_onPlaceSelected(resetSearchBox) {
    if (!this._places.hasSelection)
      return;
    var node = asQuery(this._places.selectedNode);
    LOG("Node URI: " + node.uri);
    var queries = node.getQueries({});

    
    var options = node.queryOptions.clone();
    options.excludeItems = false;
    
    
    
    options.excludeQueries = false;

    this._content.load(queries, 
                       OptionsFilter.filter(queries, options, null));
    
    
    PlacesQueryBuilder.hide();
    if (resetSearchBox) {
      var searchFilter = document.getElementById("searchFilter");
      searchFilter.reset();
    }

    this.setHeaderText(this.HEADER_TYPE_SHOWING, node.title);

    this.updateLoadedURI();

    
    
    
    var findCommand = document.getElementById("OrganizerCommand_find:current");
    var findLabel = PlacesUtils.getFormattedString("findInPrefix", [node.title]);
    findCommand.setAttribute("label", findLabel);
    if (PlacesSearchBox.filterCollection == "collection")
      PlacesSearchBox.updateCollectionTitle(node.title);
  },

  






  onTreeClick: function PO_onTreeClicked(aEvent) {
    var currentView = aEvent.currentTarget;
    var controller = currentView.controller;

    
    
    
    if (aEvent.target.localName == "treecol") {
      OptionsFilter.update(this._content.getResult());
      return;
    }
    if (currentView.hasSingleSelection && aEvent.button == 1) {
      if (PlacesUtils.nodeIsURI(currentView.selectedNode))
        controller.openSelectedNodeWithEvent(aEvent);
      else if (PlacesUtils.nodeIsContainer(currentView.selectedNode)) {
        
        
        
        controller.openLinksInTabs();
      }
    }
  },

  



  getCurrentOptions: function PO_getCurrentOptions() {
    return asQuery(this._content.getResult().root).queryOptions;
  },
  
  


  importBookmarks: function PO_import() {
    
    var features = "modal,centerscreen,chrome,resizable=no";

    
    
    window.fromFile = false;
    openDialog("chrome://browser/content/migration/migration.xul",
               "migration", features, "bookmarks");
    if (window.fromFile)
    this.importFromFile();
  },
  
  


  importFromFile: function PO_importFromFile() {
    var fp = Cc["@mozilla.org/filepicker;1"].
             createInstance(Ci.nsIFilePicker);
    fp.init(window, PlacesUtils.getString("SelectImport"),
            Ci.nsIFilePicker.modeOpen);
    fp.appendFilters(Ci.nsIFilePicker.filterHTML | Ci.nsIFilePicker.filterAll);
    if (fp.show() != Ci.nsIFilePicker.returnCancel) {
      if (fp.file) {
        var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].
                       getService(Ci.nsIPlacesImportExportService);
        var file = fp.file.QueryInterface(Ci.nsILocalFile);
        importer.importHTMLFromFile(file, false);
      }
    }
  },

  


  exportBookmarks: function PO_exportBookmarks() {
    var fp = Cc["@mozilla.org/filepicker;1"].
             createInstance(Ci.nsIFilePicker);
    fp.init(window, PlacesUtils.getString("EnterExport"),
            Ci.nsIFilePicker.modeSave);
    fp.appendFilters(Ci.nsIFilePicker.filterHTML);
    fp.defaultString = "bookmarks.html";
    if (fp.show() != Ci.nsIFilePicker.returnCancel) {
      var exporter = Cc["@mozilla.org/browser/places/import-export-service;1"].
                     getService(Ci.nsIPlacesImportExportService);
      exporter.exportHTMLToFile(fp.file);
    }
  },

  updateStatusBarForView: function PO_updateStatusBarForView(aView) {
    var statusText = "";
    var selectedNode = aView.selectedNode;
    if (selectedNode) {
      if (PlacesUtils.nodeIsFolder(selectedNode)) {
        var childsCount = 
          PlacesUtils.getFolderContents(selectedNode.itemId).root.childCount;
        statusText = PlacesUtils.getFormattedString("status_foldercount",
                                                    [childsCount]);
      }
      else if (PlacesUtils.nodeIsBookmark(selectedNode))
        statusText = selectedNode.uri;
    }
    document.getElementById("status").label = statusText;
  }
};




var PlacesSearchBox = {

  


  get searchFilter() {
    return document.getElementById("searchFilter");
  },

  






  search: function PSB_search(filterString) {
    
    
    
    
    if (PlacesSearchBox.filterCollection != "bookmarks" &&
        (filterString == "" || this.searchFilter.hasAttribute("empty")))
      return;

    var content = PlacesOrganizer._content;
    var PO = PlacesOrganizer;

    switch (PlacesSearchBox.filterCollection) {
    case "collection":
      var folderId = PlacesOrganizer._places.selectedNode.itemId;
      content.applyFilter(filterString, true, [folderId], OptionsFilter);
      PO.setHeaderText(PO.HEADER_TYPE_SEARCH, filterString);
      break;
    case "bookmarks":
      if (filterString != "")
        content.applyFilter(filterString, true);
      else
        PlacesOrganizer.onPlaceSelected();
      break;
    case "all":
      content.applyFilter(filterString, false, null, OptionsFilter);
      PO.setHeaderText(PO.HEADER_TYPE_SEARCH, filterString);
      break;
    }
    
    this.searchFilter.setAttribute("filtered", "true");
  },

  


  findAll: function PSB_findAll() {
    this.filterCollection = "all";
    this.focus();
  },

  


  findCurrent: function PSB_findCurrent() {
    this.filterCollection = "collection";
    this.focus();
  },

  




  updateCollectionTitle: function PSB_updateCollectionTitle(title) {
    if (title) {
      this.searchFilter.grayText = 
        PlacesUtils.getFormattedString("searchCurrentDefault", [title]);
    }
    else
      this.searchFilter.grayText = PlacesUtils.getString("searchByDefault");

    this.syncGrayText();
  },

  


  syncGrayText: function PSB_syncGrayText() {
    this.searchFilter.value = this.searchFilter.grayText;
  },

  


  get filterCollection() {
    return this.searchFilter.getAttribute("collection");
  },
  set filterCollection(collectionName) {
    this.searchFilter.setAttribute("collection", collectionName);
    var newGrayText = null;
    if (collectionName == "collection")
      newGrayText = PlacesOrganizer._places.selectedNode.title;
    this.updateCollectionTitle(newGrayText);
    return collectionName;
  },

  


  focus: function PSB_focus() {
    this.searchFilter.focus();
  },

  


  init: function PSB_init() {
    var searchFilter = this.searchFilter;
    searchFilter.grayText = PlacesUtils.getString("searchByDefault");
    searchFilter.reset();
  },
  
  


  get value() {
    return this.searchFilter.value;
  },
  set value(value) {
    return this.searchFilter.value = value;
  }
};




var PlacesQueryBuilder = {

  _numRows: 0,

  





  _maxRows: 3,

  _keywordSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectKeyword",
    advancedSearch_N_LocationMenulist: false,
    advancedSearch_N_TimeMenulist: false,
    advancedSearch_N_Textbox: "",
    advancedSearch_N_TimePicker: false,
    advancedSearch_N_TimeMenulist2: false
  },
  _locationSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectLocation",
    advancedSearch_N_LocationMenulist: "advancedSearch_N_LocationMenuSelected",
    advancedSearch_N_TimeMenulist: false,
    advancedSearch_N_Textbox: "",
    advancedSearch_N_TimePicker: false,
    advancedSearch_N_TimeMenulist2: false
  },
  _timeSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectVisited",
    advancedSearch_N_LocationMenulist: false,
    advancedSearch_N_TimeMenulist: true,
    advancedSearch_N_Textbox: false,
    advancedSearch_N_TimePicker: "date",
    advancedSearch_N_TimeMenulist2: false
  },
  _timeInLastSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectVisited",
    advancedSearch_N_LocationMenulist: false,
    advancedSearch_N_TimeMenulist: true,
    advancedSearch_N_Textbox: "7",
    advancedSearch_N_TimePicker: false,
    advancedSearch_N_TimeMenulist2: true
  },
  _nextSearch: null,
  _queryBuilders: null,

  init: function PQB_init() {
    
    this._nextSearch = {
      "keyword": this._timeSearch,
      "visited": this._locationSearch,
      "location": null
    };
    
    this._queryBuilders = {
      "keyword": this.setKeywordQuery,
      "visited": this.setVisitedQuery,
      "location": this.setLocationQuery
    };

    this._dateService = Cc["@mozilla.org/intl/scriptabledateformat;1"].
                        getService(Ci.nsIScriptableDateFormat);
  },

  


  hide: function PQB_hide() {
    var advancedSearch = document.getElementById("advancedSearch");
    
    advancedSearch.collapsed = true;
    
    var titleDeck = document.getElementById("titleDeck");
    titleDeck.setAttribute("selectedIndex", 0);
  },
  
  


  show: function PQB_show() {
    var advancedSearch = document.getElementById("advancedSearch");
    advancedSearch.collapsed = false;
  },

  







  _setRowId: function PQB__setRowId(element, rowId) {
    if (element.id)
      element.id = element.id.replace("advancedSearch0", "advancedSearch" + rowId);
    if (element.hasAttribute("rowid"))
      element.setAttribute("rowid", rowId);
    for (var i = 0; i < element.childNodes.length; ++i) {
      this._setRowId(element.childNodes[i], rowId);
    }
  },

  _updateUIForRowChange: function PQB__updateUIForRowChange() {
    
    var titleDeck = document.getElementById("titleDeck");
    titleDeck.setAttribute("selectedIndex", (this._numRows <= 1) ? 0 : 1);
    const asType = PlacesOrganizer.HEADER_TYPE_ADVANCED_SEARCH;
    PlacesOrganizer.setHeaderText(asType, "");

    
    
    var command = document.getElementById("OrganizerCommand_find:moreCriteria");
    if (this._numRows >= this._maxRows)
      command.setAttribute("disabled", "true");
    else
      command.removeAttribute("disabled");
  },

  



  addRow: function PQB_addRow() {
    
    
    if (this._numRows >= this._maxRows)
      return;

    
    var gridRows = document.getElementById("advancedSearchRows");
    var newRow = gridRows.firstChild.cloneNode(true);
    newRow.hidden = false;

    
    
    
    
    var searchType = this._keywordSearch;
    var lastMenu = document.getElementById("advancedSearch" +
                                           this._numRows +
                                           "Subject");
    if (this._numRows > 0 && lastMenu && lastMenu.selectedItem) {
      searchType = this._nextSearch[lastMenu.selectedItem.value];
    }
    
    if (!searchType)
      return;
    
    
    gridRows.appendChild(newRow);
    this._setRowId(newRow, ++this._numRows);

    
    
    var advancedSearch = document.getElementById("advancedSearch");
    if (advancedSearch.collapsed) {
      this.show();

      
      const asType = PlacesOrganizer.HEADER_TYPE_ADVANCED_SEARCH;
      PlacesOrganizer.setHeaderText(asType, "");
      
      
      
      
      
      var searchTermsField = document.getElementById("advancedSearch1Textbox");
      if (searchTermsField)
        setTimeout(function() { searchTermsField.value = PlacesSearchBox.value; }, 10);

      
      
      
      this.addRow();
      return;
    }      

    this.showSearch(this._numRows, searchType);
    this._updateUIForRowChange();
  },

  





  removeRow: function PQB_removeRow(row) {
    if (!row)
      row = document.getElementById("advancedSearch" + this._numRows + "Row");
    row.parentNode.removeChild(row);
    --this._numRows;

    if (this._numRows < 1) {
      this.hide();

      
      
      
      PlacesSearchBox.search(PlacesSearchBox.value);
      return;
    }

    this.doSearch();
    this._updateUIForRowChange();
  },

  onDateTyped: function PQB_onDateTyped(event, row) {
    var textbox = document.getElementById("advancedSearch" + row + "TimePicker");
    var dateString = textbox.value;
    var dateArr = dateString.split("-");
    
    
    
    
    var d0 = null;
    var d1 = null;
    
    
    if ((dateArr.length & 1) == 0) {
      var mid = dateArr.length / 2;
      var dateStr0 = dateArr[0];
      var dateStr1 = dateArr[mid];
      for (var i = 1; i < mid; ++i) {
        dateStr0 += "-" + dateArr[i];
        dateStr1 += "-" + dateArr[i + mid];
      }
      d0 = new Date(dateStr0);
      d1 = new Date(dateStr1);
    }
    
    if (d0 == null || d0 == "Invalid Date") {
      d0 = new Date(dateString);
    }

    if (d0 != null && d0 != "Invalid Date") {
      
      var calendar = document.getElementById("advancedSearch" + row + "Calendar");
      if (d0.getFullYear() < 2000)
        d0.setFullYear(2000 + (d0.getFullYear() % 100));
      if (d1 != null && d1 != "Invalid Date") {
        if (d1.getFullYear() < 2000)
          d1.setFullYear(2000 + (d1.getFullYear() % 100));
        calendar.updateSelection(d0, d1);
      }
      else {
        calendar.updateSelection(d0, d0);
      }

      
      this.doSearch();
    }
  },

  onCalendarChanged: function PQB_onCalendarChanged(event, row) {
    var calendar = document.getElementById("advancedSearch" + row + "Calendar");
    var begin = calendar.beginrange;
    var end = calendar.endrange;
    
    
    if (begin == null || end == null)
      return true;
      
    
    var textbox = document.getElementById("advancedSearch" + row + "TimePicker");
    var beginDate = begin.getDate();
    var beginMonth = begin.getMonth() + 1;
    var beginYear = begin.getFullYear();
    var endDate = end.getDate();
    var endMonth = end.getMonth() + 1;
    var endYear = end.getFullYear();
    if (beginDate == endDate && beginMonth == endMonth && beginYear == endYear) {
      
      textbox.value = this._dateService.FormatDate("",
                                                   this._dateService.dateFormatShort,
                                                   beginYear,
                                                   beginMonth,
                                                   beginDate);
    }
    else
    {
      
      var beginStr = this._dateService.FormatDate("",
                                                   this._dateService.dateFormatShort,
                                                   beginYear,
                                                   beginMonth,
                                                   beginDate);
      var endStr = this._dateService.FormatDate("",
                                                this._dateService.dateFormatShort,
                                                endYear,
                                                endMonth,
                                                endDate);
      textbox.value = beginStr + " - " + endStr;
    }
    
    
    this.doSearch();
    
    return true;
  },

  handleTimePickerClick: function PQB_handleTimePickerClick(event, row) {
    var popup = document.getElementById("advancedSearch" + row + "DatePopup");
    if (popup.showing)
      popup.hidePopup();
    else {
      var textbox = document.getElementById("advancedSearch" + row + "TimePicker");
      popup.showPopup(textbox, -1, -1, "popup", "bottomleft", "topleft");
    }
  },

  showSearch: function PQB_showSearch(row, values) {
    for (val in values) {
      var id = val.replace("_N_", row);
      var element = document.getElementById(id);
      if (values[val] || typeof(values[val]) == "string") {
        if (typeof(values[val]) == "string") {
          if (values[val] == "date") {
            
            
            var d = new Date();
            element.value = this._dateService.FormatDate("",
                                                         this._dateService.dateFormatShort,
                                                         d.getFullYear(),
                                                         d.getMonth() + 1,
                                                         d.getDate());
            var calendar = document.getElementById("advancedSearch" + row + "Calendar");
            calendar.updateSelection(d, d);
          }
          else if (element.nodeName == "textbox") {
            
            element.value = values[val];
          } else {
            
            var itemId = values[val].replace("_N_", row);
            var item = document.getElementById(itemId);
            element.selectedItem = item;
          }
        }
        element.hidden = false;
      }
      else {
        element.hidden = true;
      }
    }
    
    this.doSearch();
  },

  setKeywordQuery: function PQB_setKeywordQuery(query, prefix) {
    query.searchTerms += document.getElementById(prefix + "Textbox").value + " ";
  },

  setLocationQuery: function PQB_setLocationQuery(query, prefix) {
    var type = document.getElementById(prefix + "LocationMenulist").selectedItem.value;
    if (type == "onsite") {
      query.domain = document.getElementById(prefix + "Textbox").value;
    }
    else {
      query.uriIsPrefix = (type == "startswith");
      var ios = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
      var spec = document.getElementById(prefix + "Textbox").value;
      try {
        query.uri = ios.newURI(spec, null, null);
      }
      catch (e) {
        
        
        try {
          query.uri = ios.newURI("http://" + spec, null, null);
        }
        catch (e) {
          
          
        }
      }
    }
  },

  setVisitedQuery: function PQB_setVisitedQuery(query, prefix) {
    var searchType = document.getElementById(prefix + "TimeMenulist").selectedItem.value;
    const DAY_MSEC = 86400000;
    switch (searchType) {
      case "on":
        var calendar = document.getElementById(prefix + "Calendar");
        var begin = calendar.beginrange.getTime();
        var end = calendar.endrange.getTime();
        if (begin == end) {
          end = begin + DAY_MSEC;
        }
        query.beginTime = begin * 1000;
        query.endTime = end * 1000;
        break;
      case "before":
        var calendar = document.getElementById(prefix + "Calendar");
        var time = calendar.beginrange.getTime();
        query.endTime = time * 1000;
        break;
      case "after":
        var calendar = document.getElementById(prefix + "Calendar");
        var time = calendar.endrange.getTime();
        query.beginTime = time * 1000;
        break;
      case "inLast":
        var textbox = document.getElementById(prefix + "Textbox");
        var amount = parseInt(textbox.value);
        amount = amount * DAY_MSEC;
        var menulist = document.getElementById(prefix + "TimeMenulist2");
        if (menulist.selectedItem.value == "weeks")
          amount = amount * 7;
        else if (menulist.selectedItem.value == "months")
          amount = amount * 30;
        var now = new Date();
        now = now - amount;
        query.beginTime = now * 1000;
        break;
    }
  },

  doSearch: function PQB_doSearch() {
    
    var queryType = document.getElementById("advancedSearchType").selectedItem.value;
    var queries = [];
    if (queryType == "and")
      queries.push(PlacesUtils.history.getNewQuery());
    var updated = 0;
    for (var i = 1; updated < this._numRows; ++i) {
      var prefix = "advancedSearch" + i;

      
      
      
      var querySubjectElement = document.getElementById(prefix + "Subject");
      if (querySubjectElement) {
        
        
        var query;
        if (queryType == "and")
          query = queries[0];
        else
          query = PlacesUtils.history.getNewQuery();
        
        var querySubject = querySubjectElement.value;
        this._queryBuilders[querySubject](query, prefix);
        
        if (queryType == "or")
          queries.push(query);
          
        ++updated;
      }
    }
    
    
    var options = PlacesOrganizer.getCurrentOptions();
    options.resultType = options.RESULT_TYPE_URI;

    
    PlacesOrganizer._content.load(queries, 
                                  OptionsFilter.filter(queries, options, null));
    PlacesOrganizer.updateLoadedURI();
  }
};




var ViewMenu = {
  



















  _clean: function VM__clean(popup, startID, endID) {
    if (endID) 
      NS_ASSERT(startID, "meaningless to have valid endID and null startID");
    if (startID) {
      var startElement = document.getElementById(startID);
      NS_ASSERT(startElement.parentNode == 
                popup, "startElement is not in popup");
      NS_ASSERT(startElement, 
                "startID does not correspond to an existing element");
      var endElement = null;
      if (endID) {
        endElement = document.getElementById(endID);
        NS_ASSERT(endElement.parentNode == popup, 
                  "endElement is not in popup");
        NS_ASSERT(endElement, 
                  "endID does not correspond to an existing element");
      }
      while (startElement.nextSibling != endElement)
        popup.removeChild(startElement.nextSibling);
      return endElement;
    }
    else {
      while(popup.hasChildNodes())
        popup.removeChild(popup.firstChild);  
    }
    return null;
  },

  




















  fillWithColumns: function VM_fillWithColumns(event, startID, endID, type, propertyPrefix) {
    var popup = event.target;  
    var pivot = this._clean(popup, startID, endID);

    
    
    var isSorted = false;
    var content = document.getElementById("placeContent");
    var columns = content.columns;
    for (var i = 0; i < columns.count; ++i) {
      var column = columns.getColumnAt(i).element;
      var menuitem = document.createElementNS(XUL_NS, "menuitem");
      menuitem.id = "menucol_" + column.id;
      menuitem.setAttribute("column", column.id);
      var label = column.getAttribute("label");
      if (propertyPrefix) {
        var menuitemPrefix = propertyPrefix + column.id;
        label = PlacesUtils.getString(menuitemPrefix + ".label");
        var accesskey = PlacesUtils.getString(menuitemPrefix + ".accesskey");
        menuitem.setAttribute("accesskey", accesskey);
      }
      menuitem.setAttribute("label", label);
      if (type == "radio") {
        menuitem.setAttribute("type", "radio");
        menuitem.setAttribute("name", "columns");
        
        if (column.getAttribute("sortDirection") != "") {
          menuitem.setAttribute("checked", "true");
          isSorted = true;
        }
      }
      else if (type == "checkbox") {
        menuitem.setAttribute("type", "checkbox");
        
        if (column.primary)
          menuitem.setAttribute("disabled", "true");
        
        if (!column.hidden)
          menuitem.setAttribute("checked", "true");
      }
      if (pivot)
        popup.insertBefore(menuitem, pivot);
      else
        popup.appendChild(menuitem);      
    }
    event.stopPropagation();
  },
  
  


  populate: function VM_populate(event) {
    this.fillWithColumns(event, "viewUnsorted", "directionSeparator", "radio", "view.sortBy.");

    var sortColumn = this._getSortColumn();
    var viewSortAscending = document.getElementById("viewSortAscending");
    var viewSortDescending = document.getElementById("viewSortDescending");
    
    
    var viewUnsorted = document.getElementById("viewUnsorted");
    if (!sortColumn) {
      viewSortAscending.removeAttribute("checked");
      viewSortDescending.removeAttribute("checked");
      viewUnsorted.setAttribute("checked", "true");
    }
    else if (sortColumn.getAttribute("sortDirection") == "ascending") {
      viewSortAscending.setAttribute("checked", "true");
      viewSortDescending.removeAttribute("checked");
      viewUnsorted.removeAttribute("checked");
    }
    else if (sortColumn.getAttribute("sortDirection") == "descending") {
      viewSortDescending.setAttribute("checked", "true");
      viewSortAscending.removeAttribute("checked");
      viewUnsorted.removeAttribute("checked");
    }
  },
  
  




  showHideColumn: function VM_showHideColumn(element) {
    const PREFIX = "menucol_";
    var columnID = element.id.substr(PREFIX.length, element.id.length);
    var column = document.getElementById(columnID);
    NS_ASSERT(column, 
              "menu item for column that doesn't exist?! id = " + element.id);

    var splitter = column.nextSibling;
    if (splitter && splitter.localName != "splitter")
      splitter = null;
    
    if (element.getAttribute("checked") == "true") {
      column.removeAttribute("hidden");
      if (splitter)
        splitter.removeAttribute("hidden");
    }
    else {
      column.setAttribute("hidden", "true");
      if (splitter)
        splitter.setAttribute("hidden", "true");
    }    
  },

  



  _getSortColumn: function VM__getSortColumn() {
    var content = document.getElementById("placeContent");
    var cols = content.columns;
    for (var i = 0; i < cols.count; ++i) {
      var column = cols.getColumnAt(i).element;
      var sortDirection = column.getAttribute("sortDirection");
      if (sortDirection == "ascending" || sortDirection == "descending")
        return column;
    }
    return null;
  },

  










  setSortColumn: function VM_setSortColumn(aColumnID, aDirection) {
    var result = document.getElementById("placeContent").getResult();
    if (!aColumnID && !aDirection) {
      result.sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
      OptionsFilter.update(result);
      return;
    }

    var sortColumn = this._getSortColumn();
    if (!aDirection) {
      aDirection = sortColumn ?
                   sortColumn.getAttribute("sortDirection") : "descending";
    }
    else if (!aColumnID)
      aColumnID = sortColumn ? sortColumn.id : "title";

    var sortingMode;
    var sortingAnnotation = "";
    const NHQO = Ci.nsINavHistoryQueryOptions;
    switch (aColumnID) {
      case "title":
        sortingMode = aDirection == "descending" ?
          NHQO.SORT_BY_TITLE_DESCENDING : NHQO.SORT_BY_TITLE_ASCENDING;
        break;
      case "url":
        sortingMode = aDirection == "descending" ?
          NHQO.SORT_BY_URI_DESCENDING : NHQO.SORT_BY_URI_ASCENDING;
        break;
      case "date":
        sortingMode = aDirection == "descending" ?
          NHQO.SORT_BY_DATE_DESCENDING : NHQO.SORT_BY_DATE_ASCENDING;
        break;      
      case "visitCount":
        sortingMode = aDirection == "descending" ?
          NHQO.SORT_BY_VISITCOUNT_DESCENDING : NHQO.SORT_BY_VISITCOUNT_ASCENDING;
        break;
      case "keyword":
        sortingMode = aDirection == "descending" ?
          NHQO.SORT_BY_KEYWORD_DESCENDING : NHQO.SORT_BY_KEYWORD_ASCENDING;
        break;
      case "description":
        sortingAnnotation = DESCRIPTION_ANNO;
        sortingMode = aDirection == "descending" ?
          NHQO.SORT_BY_ANNOTATION_DESCENDING : NHQO.SORT_BY_ANNOTATION_ASCENDING;
        break;
      case "dateAdded":
        sortingMode = aDirection == "descending" ?
          NHQO.SORT_BY_DATEADDED_DESCENDING : NHQO.SORT_BY_DATEADDED_ASCENDING;
        break;
      case "lastModified":
        sortingMode = aDirection == "descending" ?
          NHQO.SORT_BY_LASTMODIFIED_DESCENDING : NHQO.SORT_BY_LASTMODIFIED_ASCENDING;
        break;
      default:
        throw("Invalid Column");
    }
    result.sortingAnnotation = sortingAnnotation;
    result.sortingMode = sortingMode;
    OptionsFilter.update(result);
  }
};

























function GroupingConfig(substr, onLabel, onAccesskey, offLabel, offAccesskey, 
                        onOncommand, offOncommand, disabled) {
  this.substr = substr;
  this.onLabel = onLabel;
  this.onAccesskey = onAccesskey;
  this.offLabel = offLabel;
  this.offAccesskey = offAccesskey;
  this.onOncommand = onOncommand;
  this.offOncommand = offOncommand;
  this.disabled = disabled;
}




var Groupers = {
  defaultGrouper: null,
  annotationGroupers: [],

  


  init: function G_init() {
    this.defaultGrouper = 
      new GroupingConfig(null, PlacesUtils.getString("defaultGroupOnLabel"),
                         PlacesUtils.getString("defaultGroupOnAccesskey"),
                         PlacesUtils.getString("defaultGroupOffLabel"),
                         PlacesUtils.getString("defaultGroupOffAccesskey"),
                         "Groupers.groupBySite()",
                         "Groupers.groupByPage()", false);
    var subscriptionConfig = 
      new GroupingConfig("livemark/", PlacesUtils.getString("livemarkGroupOnLabel"),
                         PlacesUtils.getString("livemarkGroupOnAccesskey"),
                         PlacesUtils.getString("livemarkGroupOffLabel"),
                         PlacesUtils.getString("livemarkGroupOffAccesskey"),
                         "Groupers.groupByFeed()",
                         "Groupers.groupByPost()", false);
    this.annotationGroupers.push(subscriptionConfig);
  },

  








  _getConfig: function G__getConfig(queries, handler) {
    if (!handler)
      handler = OptionsFilter.getHandler(queries);

    
    
    
    if (handler == OptionsFilter.bookmarksHandler)
      return null;

    var query = queries[0];
    for (var i = 0; i < this.annotationGroupers.length; ++i) {
      var config = this.annotationGroupers[i];
      if (query.annotation.substr(0, config.substr.length) == config.substr &&
          !config.disabled)
        return config;
    }

    return this.defaultGrouper;
  },

  











  updateGroupingUI: function G_updateGroupingUI(queries, options, handler) {
    var separator = document.getElementById("placesBC_grouping:separator");
    var groupOff = document.getElementById("placesBC_grouping:off");
    var groupOn = document.getElementById("placesBC_grouping:on");
    separator.removeAttribute("hidden");
    groupOff.removeAttribute("hidden");
    groupOn.removeAttribute("hidden");
    
    
    
    var config = this._getConfig(queries, handler);
    if (!config) {
      
      separator.setAttribute("hidden", "true");
      groupOff.setAttribute("hidden", "true");
      groupOn.setAttribute("hidden", "true");
    }
    else {
      groupOn.setAttribute("label", config.onLabel);
      groupOn.setAttribute("accesskey", config.onAccesskey);
      groupOn.setAttribute("oncommand", config.onOncommand);
      groupOff.setAttribute("label", config.offLabel);
      groupOff.setAttribute("accesskey", config.offAccesskey);
      groupOff.setAttribute("oncommand", config.offOncommand);
      
      
      
      var groupingsCountRef = { };
      options.getGroupingMode(groupingsCountRef);
      this._updateBroadcasters(groupingsCountRef.value > 0);
    }
  },

  


  _updateBroadcasters: function G__updateGroupingBroadcasters(on) {
    var groupingOn = document.getElementById("placesBC_grouping:on");
    var groupingOff = document.getElementById("placesBC_grouping:off");
    if (on) {    
      groupingOn.setAttribute("checked", "true");
      groupingOff.removeAttribute("checked");
    }
    else {
      groupingOff.setAttribute("checked", "true");
      groupingOn.removeAttribute("checked");
    }
  },

  


  groupBySite: function G_groupBySite() {
    var query = asQuery(PlacesOrganizer._content.getResult().root);
    var queries = query.getQueries({ });
    var options = query.queryOptions;
    var newOptions = options.clone();
    const NHQO = Ci.nsINavHistoryQueryOptions;
    newOptions.setGroupingMode([NHQO.GROUP_BY_DOMAIN], 1);
    var content = PlacesOrganizer._content;
    content.load(queries, newOptions);
    PlacesOrganizer.updateLoadedURI();
    this._updateBroadcasters(true);
    OptionsFilter.update(content.getResult());
  },

  


  groupByPage: function G_groupByPage() {
    var query = asQuery(PlacesOrganizer._content.getResult().root);
    var queries = query.getQueries({ });
    var options = query.queryOptions;
    var newOptions = options.clone();
    newOptions.setGroupingMode([], 0);
    var content = PlacesOrganizer._content;
    content.load(queries, newOptions);
    PlacesOrganizer.updateLoadedURI();
    this._updateBroadcasters(false);
    OptionsFilter.update(content.getResult());
  },

  



  groupByFeed: function G_groupByFeed() {
    var content = PlacesOrganizer._content;
    var query = asQuery(content.getResult().root);
    var queries = query.getQueries({ });
    var newOptions = query.queryOptions.clone();
    var newQuery = queries[0].clone();
    newQuery.annotation = "livemark/feedURI";
    content.load([newQuery], newOptions);
    PlacesOrganizer.updateLoadedURI();
    this._updateBroadcasters(false);
    OptionsFilter.update(content.getResult());
  },

  


  groupByPost: function G_groupByPost() {
    var content = PlacesOrganizer._content;
    var query = asQuery(content.getResult().root);
    var queries = query.getQueries({ });
    var newOptions = query.queryOptions.clone();
    var newQuery = queries[0].clone();
    newQuery.annotation = "livemark/bookmarkFeedURI";
    content.load([newQuery], newOptions);
    PlacesOrganizer.updateLoadedURI();
    this._updateBroadcasters(false);
    OptionsFilter.update(content.getResult());
  }
};

