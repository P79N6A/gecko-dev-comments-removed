#ifdef 0



#endif




let gDrag = {
  


  _offsetX: null,
  _offsetY: null,

  


  _draggedSite: null,
  get draggedSite() this._draggedSite,

  


  _cellWidth: null,
  _cellHeight: null,
  get cellWidth() this._cellWidth,
  get cellHeight() this._cellHeight,

  




  start: function Drag_start(aSite, aEvent) {
    this._draggedSite = aSite;

    
    aSite.node.setAttribute("dragged", "true");

    
    aSite.node.setAttribute("ontop", "true");

    this._setDragData(aSite, aEvent);

    
    let node = aSite.node;
    let rect = node.getBoundingClientRect();
    this._offsetX = aEvent.clientX - rect.left;
    this._offsetY = aEvent.clientY - rect.top;

    
    let cellNode = aSite.cell.node;
    this._cellWidth = cellNode.offsetWidth;
    this._cellHeight = cellNode.offsetHeight;

    gTransformation.freezeSitePosition(aSite);
  },

  




  drag: function Drag_drag(aSite, aEvent) {
    
    let {clientWidth, clientHeight} = document.documentElement;

    
    let border = 5;

    
    let left = Math.max(scrollX + aEvent.clientX - this._offsetX, border);
    let top = Math.max(scrollY + aEvent.clientY - this._offsetY, border);

    
    left = Math.min(left, scrollX + clientWidth - this.cellWidth - border);
    top = Math.min(top, scrollY + clientHeight - this.cellHeight - border);

    
    gTransformation.setSitePosition(aSite, {left: left, top: top});
  },

  




  end: function Drag_end(aSite, aEvent) {
    aSite.node.removeAttribute("dragged");

    
    gTransformation.slideSiteTo(aSite, aSite.cell, {
      unfreeze: true,
      callback: function () aSite.node.removeAttribute("ontop")
    });

    this._draggedSite = null;
  },

  




  isValid: function Drag_isValid(aEvent) {
    let dt = aEvent.dataTransfer;
    return dt && dt.types.contains("text/x-moz-url");
  },

  




  _setDragData: function Drag_setDragData(aSite, aEvent) {
    let {url, title} = aSite;

    let dt = aEvent.dataTransfer;
    dt.mozCursor = "default";
    dt.effectAllowed = "move";
    dt.setData("text/plain", url);
    dt.setData("text/uri-list", url);
    dt.setData("text/x-moz-url", url + "\n" + title);
    dt.setData("text/html", "<a href=\"" + url + "\">" + url + "</a>");

    
    
    let dragElement = document.createElementNS(HTML_NAMESPACE, "div");
    dragElement.classList.add("drag-element");
    let body = document.getElementById("body");
    body.appendChild(dragElement);
    dt.setDragImage(dragElement, 0, 0);

    
    
    setTimeout(function () body.removeChild(dragElement), 0);
  }
};
