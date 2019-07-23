




































var BookmarksMenu = {
  _selection:null,
  _target:null,
  _orientation:null,

  
  
  
  createContextMenu: function (aEvent)
  {
    var target = document.popupNode;
    if (!this.isBTBookmark(target.id) && target.id != "bookmarks-ptf")
      return false;
    target.focus() 
    this._selection   = this.getBTSelection(target);
    this._orientation = this.getBTOrientation(aEvent, target);
    this._target      = this.getBTTarget(target, this._orientation);
    BookmarksCommand.createContextMenu(aEvent, this._selection);
    this.onCommandUpdate();
    aEvent.target.addEventListener("mousemove", BookmarksMenuController.onMouseMove, false);
    return true;
  },

  
  
  destroyContextMenu: function (aEvent)
  {
    if (content)
      content.focus();
    BookmarksMenuDNDObserver.onDragRemoveFeedBack(document.popupNode); 
    aEvent.target.removeEventListener("mousemove", BookmarksMenuController.onMouseMove, false);
  },

  
  
  getBTSelection: function (aNode)
  {
    var item;
    switch (aNode.id) {
    case "bookmarks-ptf":
      item = "NC:PersonalToolbarFolder";
      break;
    case "BookmarksMenu":
      item = "NC:BookmarksRoot";
      break;
    default:
      item = aNode.id;
    }
    if (!this.isBTBookmark(item))
      return {length:0};
    var parent           = this.getBTContainer(aNode);
    var isExpanded       = aNode.hasAttribute("open") && aNode.open;
    var selection        = {};
    selection.item       = [RDF.GetResource(item)];
    selection.parent     = [RDF.GetResource(parent)];
    selection.isExpanded = [isExpanded];
    selection.length     = selection.item.length;
    BookmarksUtils.checkSelection(selection);
    return selection;
  },

  
  
  getBTTarget: function (aNode, aOrientation)
  {
    var item, parent, index;

    switch (aNode.id) {
    case "BookmarksMenu":
    case "bookmarks-button":
      parent = "NC:BookmarksRoot";
      break;
    case "bookmarks-ptf":
      item = BookmarksToolbar.getLastVisibleBookmark();
      
    case "bookmarks-chevron":
      parent = "NC:PersonalToolbarFolder";
      break;
    default:
      if (aOrientation == BookmarksUtils.DROP_ON)
        parent = aNode.id
      else {
        parent = this.getBTContainer(aNode);
        item = aNode;
      }
    }

    parent = RDF.GetResource(parent);
    if (aOrientation == BookmarksUtils.DROP_ON)
      return BookmarksUtils.getTargetFromFolder(parent);

    item = RDF.GetResource(item.id);
    RDFC.Init(BMDS, parent);
    index = RDFC.IndexOf(item);
    if (aOrientation == BookmarksUtils.DROP_AFTER)
      ++index;

    return { parent: parent, index: index };
  },

  
  
  
  
  getBTContainer: function (aNode)
  {
    var parent;
    var item = aNode.id;
    if (!this.isBTBookmark(item))
      return "NC:BookmarksRoot"
    parent = aNode.parentNode.parentNode;
    parent = parent.id;
    switch (parent) {
    case "BookmarksMenu":
    case "bookmarks-button":
      return "NC:BookmarksRoot";
    case "PersonalToolbar":
    case "bookmarks-chevron":
      return "NC:PersonalToolbarFolder";
    default:
      return parent;
    }
  },

  
  
  isBTBookmark: function (aURI)
  {
    if (!aURI)
      return false;
    var type = BookmarksUtils.resolveType(aURI);
    return (type == "BookmarkSeparator" ||
            type == "Bookmark"          ||
            type == "Folder"            ||
            type == "FolderGroup"       ||
            type == "PersonalToolbarFolder")
  },

  
  
  isBTContainer: function (aTarget)
  {
    return  aTarget.localName == "menu" || (aTarget.localName == "toolbarbutton" &&
           (aTarget.getAttribute("container") == "true" || aTarget.getAttribute("group") == "true"));
  },

  
  
  
  
  getBTOrientation: function (aEvent, aTarget)
  {
    var target
    if (!aTarget)
      target = aEvent.target;
    else
      target = aTarget;
    if (target.localName == "menu"                 &&
        target.parentNode.localName != "menupopup")
      return BookmarksUtils.DROP_ON;
    if (target.id == "bookmarks-ptf") {
      return target.hasChildNodes() ?
          BookmarksUtils.DROP_AFTER : BookmarksUtils.DROP_ON;
    }
    if (target.id == "bookmarks-chevron")
      return BookmarksUtils.DROP_ON;

    var overButtonBoxObject = target.boxObject.QueryInterface(Components.interfaces.nsIBoxObject);
    var overParentBoxObject = target.parentNode.boxObject.QueryInterface(Components.interfaces.nsIBoxObject);

    var size, border;
    var coordValue, clientCoordValue;
    switch (target.localName) {
      case "toolbarseparator":
      case "toolbarbutton":
        size = overButtonBoxObject.width;
        coordValue = overButtonBoxObject.x;
        clientCoordValue = aEvent.clientX;
        break;
      case "menuseparator": 
      case "menu":
      case "menuitem":
        size = overButtonBoxObject.height;
        coordValue = overButtonBoxObject.screenY;
        clientCoordValue = aEvent.screenY;
        break;
      default: return BookmarksUtils.DROP_ON;
    }
    if (this.isBTContainer(target))
      if (target.localName == "toolbarbutton") {
        
        var iconNode = document.getAnonymousElementByAttribute(target, "class", "toolbarbutton-icon");
        border = parseInt(document.defaultView.getComputedStyle(target,"").getPropertyValue("padding-left")) +
                 parseInt(document.defaultView.getComputedStyle(iconNode     ,"").getPropertyValue("width"));
        border = Math.min(size/5,Math.max(border,4));
      } else
        border = size/5;
    else
      border = size/2;

    
    if (clientCoordValue-coordValue < border)
      return BookmarksUtils.DROP_BEFORE;
    
    if (clientCoordValue-coordValue >= size-border)
      return BookmarksUtils.DROP_AFTER;
    
    return BookmarksUtils.DROP_ON;
  },

  
  
  expandBTFolder: function ()
  {
    var target = document.popupNode.lastChild;
    if (document.popupNode.open)
      target.hidePopup();
    else
      target.showPopup(document.popupNode);
  },

  onCommandUpdate: function ()
  {
    var selection = this._selection;
    var target    = this._target;
    BookmarksController.onCommandUpdate(selection, target);
    if (document.popupNode.id == "bookmarks-ptf") {
      
      var commandNode = document.getElementById("cmd_bm_cut");
      commandNode.setAttribute("disabled", "true");
      commandNode = document.getElementById("cmd_bm_copy");
      commandNode.setAttribute("disabled", "true");
    }
  },

  loadBookmark: function (aEvent, aDS)
  {
    if (this.isBTBookmark(aEvent.target.id))
      BookmarksUtils.loadBookmarkBrowser(aEvent, aDS);
  },

  
  
  loadBookmarkMiddleClick: function (aEvent, aDS)
  {
    if (aEvent.type != "click" || aEvent.button != 1)
      return;
    
    for (var node = aEvent.target; node != aEvent.currentTarget;
         node = node.parentNode) {
      if (node.nodeType == node.ELEMENT_NODE && node.tagName == "menupopup")
        node.hidePopup();
    }
    this.loadBookmark(aEvent, aDS);
  }
}

var BookmarksMenuController = {

  supportsCommand: BookmarksController.supportsCommand,

  isCommandEnabled: function (aCommand)
  {
    
    var selection = BookmarksMenu._selection;
    var target    = BookmarksMenu._target;
    if (selection)
      return BookmarksController.isCommandEnabled(aCommand, selection, target);
    return false;
  },

  doCommand: function (aCommand)
  {
    BookmarksMenuDNDObserver.onDragRemoveFeedBack(document.popupNode);
    var selection = BookmarksMenu._selection;
    var target    = BookmarksMenu._target;
    switch (aCommand) {
    case "cmd_bm_expandfolder":
      setTimeout(BookmarksMenu.expandBTFolder, 0);
      break;
    default:
      BookmarksController.doCommand(aCommand, selection, target);
    }
  },

  onMouseMove: function (aEvent)
  {
    var command = aEvent.target.getAttribute("command");
    var isDisabled = aEvent.target.getAttribute("disabled")
    if (isDisabled != "true" && (command == "cmd_bm_newfolder" || command == "cmd_bm_paste")) {
      BookmarksMenuDNDObserver.onDragSetFeedBack(document.popupNode, BookmarksMenu._orientation);
    } else {
      BookmarksMenuDNDObserver.onDragRemoveFeedBack(document.popupNode);
    }
  }
}

var BookmarksMenuDNDObserver = {

  
  
  

  onDragStart: function (aEvent, aXferData, aDragAction)
  {
    var target = aEvent.target;

    
    if (!this.canDrop(aEvent))
      return;

    
    
    
    
    
    if (navigator.platform != "Win32" && target.localName != "toolbarbutton")
      return;

    
    if (target.id == "bookmarks-ptf")
      return

    
    
    if (this.isContainer(target) && 
        target.getAttribute("group") != "true") {
      if (this.isPlatformNotSupported) 
        return;
      if (!aEvent.shiftKey && !aEvent.altKey && !aEvent.ctrlKey)
        return;
      
      target.firstChild.hidePopup();
    }
    var selection  = BookmarksMenu.getBTSelection(target);
    aXferData.data = BookmarksUtils.getXferDataFromSelection(selection);
  },

  onDragOver: function(aEvent, aFlavour, aDragSession) 
  {
    var orientation = BookmarksMenu.getBTOrientation(aEvent)
    if (aDragSession.canDrop)
      this.onDragSetFeedBack(aEvent.target, orientation);
    if (orientation != this.mCurrentDropPosition) {
      
      
      this.onDragExit(aEvent, aDragSession);
      this.onDragEnter(aEvent, aDragSession);
    }
    if (this.isPlatformNotSupported)
      return;
    if (this.isTimerSupported || !aDragSession.sourceNode)
      return;
    this.onDragOverCheckTimers();
  },

  onDragEnter: function (aEvent, aDragSession)
  {
    var target = aEvent.target;
    var orientation = BookmarksMenu.getBTOrientation(aEvent);
    if (target.localName == "menupopup" || target.id == "bookmarks-ptf")
      target = target.parentNode;
    if (aDragSession.canDrop) {
      this.onDragSetFeedBack(target, orientation);
      this.onDragEnterSetTimer(target, aDragSession);
    }
    this.mCurrentDragOverTarget = target;
    this.mCurrentDropPosition   = orientation;
  },

  onDragExit: function (aEvent, aDragSession)
  {
    var target = aEvent.target;
    if (target.localName == "menupopup" || target.id == "bookmarks-ptf")
      target = target.parentNode;
    this.onDragRemoveFeedBack(target);
    this.onDragExitSetTimer(target, aDragSession);
    this.mCurrentDragOverTarget = null;
    this.mCurrentDropPosition = null;
  },

  onDrop: function (aEvent, aXferData, aDragSession)
  {
    var target = aEvent.target;
    this.onDragRemoveFeedBack(target);

    var selection = BookmarksUtils.getSelectionFromXferData(aDragSession);

    var orientation = BookmarksMenu.getBTOrientation(aEvent);

    
    
    if (target.localName == "toolbarbutton")
      if (window.getComputedStyle(document.getElementById("PersonalToolbar"),'').direction == 'rtl')
        if (orientation == BookmarksUtils.DROP_AFTER)
          orientation = BookmarksUtils.DROP_BEFORE;
        else if (orientation == BookmarksUtils.DROP_BEFORE)
          orientation = BookmarksUtils.DROP_AFTER;

    var selTarget   = BookmarksMenu.getBTTarget(target, orientation);

    const kDSIID      = Components.interfaces.nsIDragService;
    const kCopyAction = kDSIID.DRAGDROP_ACTION_COPY + kDSIID.DRAGDROP_ACTION_LINK;

    
    
    
    var menuTarget = (target.localName == "toolbarbutton" ||
                      target.localName == "menu")         && 
                     orientation == BookmarksUtils.DROP_ON?
                     target.lastChild:target.parentNode;
    if (menuTarget.hasChildNodes() &&
        menuTarget.lastChild.id == "openintabs-menuitem") {
      menuTarget.removeChild(menuTarget.lastChild.previousSibling);
    }

    if (aDragSession.dragAction & kCopyAction)
      BookmarksUtils.insertSelection("drag", selection, selTarget);
    else
      BookmarksUtils.moveSelection("drag", selection, selTarget);

    var chevron = document.getElementById("bookmarks-chevron");
    if (chevron.getAttribute("open") == "true") {
      BookmarksToolbar.resizeFunc(null);
      BookmarksToolbar.updateOverflowMenu(document.getElementById("bookmarks-chevron-popup"));
    }

    
    if (menuTarget.hasChildNodes() &&
        menuTarget.lastChild.id == "openintabs-menuitem") {
      var element = document.createElementNS(XUL_NS, "menuseparator");
      menuTarget.insertBefore(element, menuTarget.lastChild);
    }
  },

  canDrop: function (aEvent, aDragSession)
  {
    var target = aEvent.target;

    
    
    if (aDragSession && aDragSession.sourceNode) {
      var orientation = BookmarksMenu.getBTOrientation(aEvent, target);
      if (target == aDragSession.sourceNode ||
          (target == aDragSession.sourceNode.previousSibling &&
           orientation == BookmarksUtils.DROP_AFTER) ||
          (target == aDragSession.sourceNode.nextSibling &&
           orientation == BookmarksUtils.DROP_BEFORE))
        return false;
    }

    return BookmarksMenu.isBTBookmark(target.id)       && 
           target.id != "NC:SystemBookmarksStaticRoot" &&
           target.id.substring(0,5) != "find:"         ||
           target.id == "BookmarksMenu"                ||
           target.id == "bookmarks-button"             ||
           target.id == "bookmarks-chevron"            ||
           target.id == "bookmarks-ptf";
  },

  canHandleMultipleItems: true,

  getSupportedFlavours: function () 
  {
    var flavourSet = new FlavourSet();
    flavourSet.appendFlavour("moz/rdfitem");
    flavourSet.appendFlavour("text/x-moz-url");
    flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
    flavourSet.appendFlavour("text/unicode");
    return flavourSet;
  },


  
  
  

  springLoadedMenuDelay: 350, 
  isPlatformNotSupported: navigator.platform.indexOf("Mac") != -1, 
  
  isTimerSupported: navigator.platform.indexOf("Win") == -1,

  mCurrentDragOverTarget: null,
  mCurrentDropPosition: null,
  loadTimer  : null,
  closeTimer : null,
  loadTarget : null,
  closeTarget: null,

  _observers : null,
  get mObservers ()
  {
    if (!this._observers) {
      this._observers = [
        document.getElementById("bookmarks-ptf"),
        document.getElementById("BookmarksMenu").parentNode,
        document.getElementById("bookmarks-chevron").parentNode,
        document.getElementById("PersonalToolbar")
      ]
    }
    return this._observers;
  },

  getObserverForNode: function (aNode)
  {
    if (!aNode)
      return null;
    var node = aNode;
    var observer;
    do {
      for (var i=0; i < this.mObservers.length; i++) {
        observer = this.mObservers[i];
        if (observer == node)
          return observer;
      }
      node = node.parentNode;
    } while (node != document)
    return null;
  },

  onDragCloseMenu: function (aNode)
  {
    var children = aNode.childNodes;
    for (var i = 0; i < children.length; i++) {
      if (this.isContainer(children[i]) && 
          children[i].getAttribute("open") == "true") {
        this.onDragCloseMenu(children[i].lastChild);
        if (children[i] != this.mCurrentDragOverTarget || this.mCurrentDropPosition != BookmarksUtils.DROP_ON)
          children[i].lastChild.hidePopup();
      }
    }
  },

  onDragCloseTarget: function ()
  {
    var currentObserver = this.getObserverForNode(this.mCurrentDragOverTarget);
    
    for (var i=0; i < this.mObservers.length; i++) {
      if (currentObserver != this.mObservers[i])
        this.onDragCloseMenu(this.mObservers[i]);
      else
        this.onDragCloseMenu(this.mCurrentDragOverTarget.parentNode);
    }
  },

  onDragLoadTarget: function (aTarget) 
  {
    if (!this.mCurrentDragOverTarget)
      return;
    
    if (this.mCurrentDropPosition == BookmarksUtils.DROP_ON && 
        this.isContainer(aTarget)             && 
        aTarget.getAttribute("group") != "true")
      aTarget.lastChild.showPopup(aTarget);
  },

  onDragOverCheckTimers: function ()
  {
    var now = new Date().getTime();
    if (this.closeTimer && now-this.springLoadedMenuDelay>this.closeTimer) {
      this.onDragCloseTarget();
      this.closeTimer = null;
    }
    if (this.loadTimer && (now-this.springLoadedMenuDelay>this.loadTimer)) {
      this.onDragLoadTarget(this.loadTarget);
      this.loadTimer = null;
    }
  },

  onDragEnterSetTimer: function (aTarget, aDragSession)
  {
    if (this.isPlatformNotSupported)
      return;
    if (this.isTimerSupported || !aDragSession.sourceNode) {
      var targetToBeLoaded = aTarget;
      clearTimeout(this.loadTimer);
      if (aTarget == aDragSession.sourceNode)
        return;
      var This = this;
      this.loadTimer=setTimeout(function () {This.onDragLoadTarget(targetToBeLoaded)}, This.springLoadedMenuDelay);
    } else {
      var now = new Date().getTime();
      this.loadTimer  = now;
      this.loadTarget = aTarget;
    }
  },

  onDragExitSetTimer: function (aTarget, aDragSession)
  {
    if (this.isPlatformNotSupported)
      return;
    var This = this;
    if (this.isTimerSupported || !aDragSession.sourceNode) {
      clearTimeout(this.closeTimer)
      this.closeTimer=setTimeout(function () {This.onDragCloseTarget()}, This.springLoadedMenuDelay);
    } else {
      var now = new Date().getTime();
      this.closeTimer  = now;
      this.closeTarget = aTarget;
      this.loadTimer = null;

      
      
      
      
      
      
      
      if (aDragSession.sourceNode.localName != "menuitem" && aDragSession.sourceNode.localName != "menu")
        setTimeout(function () { if (This.mCurrentDragOverTarget) {This.onDragRemoveFeedBack(This.mCurrentDragOverTarget); This.mCurrentDragOverTarget=null} This.loadTimer=null; This.onDragCloseTarget() }, 0);
    }
  },

  onDragSetFeedBack: function (aTarget, aOrientation)
  {
   switch (aTarget.localName) {
      case "toolbarseparator":
      case "toolbarbutton":
        switch (aOrientation) {
          case BookmarksUtils.DROP_BEFORE: 
            aTarget.setAttribute("dragover-left", "true");
            break;
          case BookmarksUtils.DROP_AFTER:
            aTarget.setAttribute("dragover-right", "true");
            break;
          case BookmarksUtils.DROP_ON:
            aTarget.setAttribute("dragover-top"   , "true");
            aTarget.setAttribute("dragover-bottom", "true");
            aTarget.setAttribute("dragover-left"  , "true");
            aTarget.setAttribute("dragover-right" , "true");
            break;
        }
        break;
      case "menuseparator": 
      case "menu":
      case "menuitem":
        switch (aOrientation) {
          case BookmarksUtils.DROP_BEFORE: 
            aTarget.setAttribute("dragover-top", "true");
            break;
          case BookmarksUtils.DROP_AFTER:
            aTarget.setAttribute("dragover-bottom", "true");
            break;
          case BookmarksUtils.DROP_ON:
            break;
        }
        break;
      case "toolbar": 
        var newTarget = BookmarksToolbar.getLastVisibleBookmark();
        if (newTarget)
          newTarget.setAttribute("dragover-right", "true");
        break;
      case "hbox":
      case "menupopup": break; 
     default: dump("No feedback for: "+aTarget.localName+"\n");
    }
  },

  onDragRemoveFeedBack: function (aTarget)
  {
    var newTarget;
    var bt;
    if (aTarget.id == "PersonalToolbar" || aTarget.id == "bookmarks-ptf") { 
      newTarget = BookmarksToolbar.getLastVisibleBookmark();
      if (newTarget)
        newTarget.removeAttribute("dragover-right");
    } else {
      aTarget.removeAttribute("dragover-left");
      aTarget.removeAttribute("dragover-right");
      aTarget.removeAttribute("dragover-top");
      aTarget.removeAttribute("dragover-bottom");
    }
  },

  onDropSetFeedBack: function (aTarget)
  {
    
  },

  isContainer: function (aTarget)
  {
    return aTarget.localName == "menu"          || 
           aTarget.localName == "toolbarbutton" &&
           aTarget.getAttribute("type") == "menu";
  }
}

var BookmarksToolbar = 
{
  
  
  openMenuButton: null,
  autoOpenMenu: function (aTarget)
  {
    if (this.openMenuButton &&
        this.openMenuButton != aTarget &&
        aTarget.localName == "toolbarbutton" &&
        (aTarget.type == "menu" ||
         aTarget.type == "menu-button")) {
      this.openMenuButton.open = false;
      aTarget.open = true;
    }
  },
  onMenuOpen: function (aTarget)
  {
    if (aTarget.parentNode.localName == "toolbarbutton")
      this.openMenuButton = aTarget.parentNode;
  },
  onMenuClose: function (aTarget)
  {
    if (aTarget.parentNode.localName == "toolbarbutton")
      this.openMenuButton = null;
  },

  
  
  getLastVisibleBookmark: function ()
  {
    var buttons = document.getElementById("bookmarks-ptf");
    var button = buttons.firstChild;
    if (!button)
      return null; 
    do {
      if (button.collapsed)
        return button.previousSibling;
      button = button.nextSibling;
    } while (button)
    return buttons.lastChild;
  },

  updateOverflowMenu: function (aMenuPopup)
  {
    var hbox = document.getElementById("bookmarks-ptf");
    for (var i = 0; i < hbox.childNodes.length; i++) {
      var button = hbox.childNodes[i];
      var menu = aMenuPopup.childNodes[i];
      if (menu.hidden == button.collapsed)
        menu.hidden = !menu.hidden;
    }
  },

  resizeFunc: function(event) 
  {
    if (!event) 
      BookmarksToolbarRDFObserver._overflowTimerInEffect = false;
    else if (event.target != window)
      return; 

    var buttons = document.getElementById("bookmarks-ptf");
    if (!buttons)
      return;

    var chevron = document.getElementById("bookmarks-chevron");
    if (!buttons.firstChild) {
      
      chevron.collapsed = true;
      return;
    }

    chevron.collapsed = false;
    var chevronWidth = chevron.boxObject.width;
    chevron.collapsed = true;

    var remainingWidth = buttons.boxObject.width;
    var overflowed = false;

    for (var i=0; i<buttons.childNodes.length; i++) {
      var button = buttons.childNodes[i];
      button.collapsed = overflowed;

      if (i == buttons.childNodes.length - 1)
        chevronWidth = 0;
      remainingWidth -= button.boxObject.width;
      if (remainingWidth < chevronWidth) {
        overflowed = true;
        
        if (!button.collapsed)
          button.collapsed = true;
        if (chevron.collapsed) {
          chevron.collapsed = false;
        }
      }
    }
  },

  
  fillInBTTooltip: function (tipElement)
  {
    
    if (!/bookmark/.test(tipElement.className))
      return false;

    var title = tipElement.label;
    var url = tipElement.statusText;

    
    if (!title && !url)
      return false;

    var tooltipElement = document.getElementById("btTitleText");
    tooltipElement.hidden = !title || (title == url);
    if (!tooltipElement.hidden)
      tooltipElement.setAttribute("value", title);

    tooltipElement = document.getElementById("btUrlText");
    tooltipElement.hidden = !url;
    if (!tooltipElement.hidden)
      tooltipElement.setAttribute("value", url);

    
    return true;
  }
}



var BookmarksToolbarRDFObserver =
{
  onAssert: function (aDataSource, aSource, aProperty, aTarget)
  {
    this.setOverflowTimeout(aSource, aProperty);
  },
  onUnassert: function (aDataSource, aSource, aProperty, aTarget)
  {
    this.setOverflowTimeout(aSource, aProperty);
  },
  onChange: function (aDataSource, aSource, aProperty, aOldTarget, aNewTarget) {},
  onMove: function (aDataSource, aOldSource, aNewSource, aProperty, aTarget) {},
  onBeginUpdateBatch: function (aDataSource) {},
  onEndUpdateBatch: function (aDataSource)
  {
    if (this._overflowTimerInEffect)
      return;
    this._overflowTimerInEffect = true;
    setTimeout(BookmarksToolbar.resizeFunc, 0, null);
  },
  _overflowTimerInEffect: false,
  setOverflowTimeout: function (aSource, aProperty)
  {
    if (this._overflowTimerInEffect)
      return;
    if (aSource.Value != "NC:PersonalToolbarFolder" || aProperty.Value == NC_NS+"LastModifiedDate")
      return;
    this._overflowTimerInEffect = true;
    setTimeout(BookmarksToolbar.resizeFunc, 0, null);
  }
}
