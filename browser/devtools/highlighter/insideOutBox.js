






























































































































function InsideOutBox(aView, aBox)
{
  this.view = aView;
  this.box = aBox;

  this.rootObject = null;

  this.rootObjectBox = null;
  this.selectedObjectBox = null;
  this.highlightedObjectBox = null;
  this.scrollIntoView = false;
};

InsideOutBox.prototype =
{
  





  highlight: function IOBox_highlight(aObject)
  {
    let objectBox = this.createObjectBox(aObject);
    this.highlightObjectBox(objectBox);
    return objectBox;
  },

  





  openObject: function IOBox_openObject(aObject)
  {
    let object = aObject;
    let firstChild = this.view.getChildObject(object, 0);
    if (firstChild)
      object = firstChild;

    return this.openToObject(object);
  },

  





  openToObject: function IOBox_openToObject(aObject)
  {
    let objectBox = this.createObjectBox(aObject);
    this.openObjectBox(objectBox);
    return objectBox;
  },

  















  select:
  function IOBox_select(aObject, makeBoxVisible, forceOpen, scrollIntoView)
  {
    let objectBox = this.createObjectBox(aObject);
    if (!objectBox) {
      return null;
    }
    this.selectObjectBox(objectBox, forceOpen);
    if (makeBoxVisible) {
      this.openObjectBox(objectBox);
      if (scrollIntoView) {
        objectBox.scrollIntoView(true);
      }
    }
    return objectBox;
  },

  




  toggleObject: function IOBox_toggleObject(aObject)
  {
    let box = this.createObjectBox(aObject);
    if (!(this.view.hasClass(box, "open")))
      this.expandObjectBox(box);
    else
      this.contractObjectBox(box);
  },

  




  expandObject: function IOBox_expandObject(aObject)
  {
    let objectBox = this.createObjectBox(aObject);
    if (objectBox)
      this.expandObjectBox(objectBox);
  },

  




  contractObject: function IOBox_contractObject(aObject)
  {
    let objectBox = this.createObjectBox(aObject);
    if (objectBox)
      this.contractObjectBox(objectBox);
  },

  








  iterateObjectAncestors: function IOBox_iterateObjectAncesors(aObject, aCallback)
  {
    let object = aObject;
    if (!(aCallback && typeof(aCallback) == "function")) {
      this.view._log("Illegal argument in IOBox.iterateObjectAncestors");
      return;
    }
    while ((object = this.getParentObjectBox(object)))
      aCallback(object);
  },

  




  highlightObjectBox: function IOBox_highlightObjectBox(aObjectBox)
  {
    let self = this;

    if (!aObjectBox)
      return;

    if (this.highlightedObjectBox) {
      this.view.removeClass(this.highlightedObjectBox, "highlighted");
      this.iterateObjectAncestors(this.highlightedObjectBox, function (box) {
        self.view.removeClass(box, "highlightOpen");
      });
    }

    this.highlightedObjectBox = aObjectBox;

    this.view.addClass(aObjectBox, "highlighted");
    this.iterateObjectAncestors(this.highlightedObjectBox, function (box) {
      self.view.addClass(box, "highlightOpen");
    });

    aObjectBox.scrollIntoView(true);
  },

  






  selectObjectBox: function IOBox_selectObjectBox(aObjectBox, forceOpen)
  {
    let isSelected = this.selectedObjectBox &&
      aObjectBox == this.selectedObjectBox;

    
    if (isSelected)
      return;

    if (this.selectedObjectBox)
      this.view.removeClass(this.selectedObjectBox, "selected");

    this.selectedObjectBox = aObjectBox;

    if (aObjectBox) {
      this.view.addClass(aObjectBox, "selected");

      
      if (forceOpen)
        this.expandObjectBox(aObjectBox, true);
    }
  },

  




  openObjectBox: function IOBox_openObjectBox(aObjectBox)
  {
    if (!aObjectBox)
      return;

    let self = this;
    this.iterateObjectAncestors(aObjectBox, function (box) {
      self.view.addClass(box, "open");
      let labelBox = box.querySelector(".nodeLabelBox");
      if (labelBox)
        labelBox.setAttribute("aria-expanded", "true");
   });
  },

  




  expandObjectBox: function IOBox_expandObjectBox(aObjectBox)
  {
    let nodeChildBox = this.getChildObjectBox(aObjectBox);

    
    if (!nodeChildBox)
      return;

    if (!aObjectBox.populated) {
      let firstChild = this.view.getChildObject(aObjectBox.repObject, 0);
      this.populateChildBox(firstChild, nodeChildBox);
    }
    let labelBox = aObjectBox.querySelector(".nodeLabelBox");
    if (labelBox)
      labelBox.setAttribute("aria-expanded", "true");
    this.view.addClass(aObjectBox, "open");
  },

  




  contractObjectBox: function IOBox_contractObjectBox(aObjectBox)
  {
    this.view.removeClass(aObjectBox, "open");
    let nodeLabel = aObjectBox.querySelector(".nodeLabel");
    let labelBox = nodeLabel.querySelector(".nodeLabelBox");
    if (labelBox)
      labelBox.setAttribute("aria-expanded", "false");
  },

  






  toggleObjectBox: function IOBox_toggleObjectBox(aObjectBox, forceOpen)
  {
    let isOpen = this.view.hasClass(aObjectBox, "open");

    if (!forceOpen && isOpen)
      this.contractObjectBox(aObjectBox);
    else if (!isOpen)
      this.expandObjectBox(aObjectBox);
  },

  





  createObjectBox: function IOBox_createObjectBox(aObject)
  {
    if (!aObject)
      return null;

    this.rootObject = this.getRootNode(aObject) || aObject;

    
    let objectBox = this.createObjectBoxes(aObject, this.rootObject);

    if (!objectBox)
      return null;

    if (aObject == this.rootObject)
      return objectBox;

    return this.populateChildBox(aObject, objectBox.parentNode);
  },

  








  createObjectBoxes: function IOBox_createObjectBoxes(aObject, aRootObject)
  {
    if (!aObject)
      return null;

    if (aObject == aRootObject) {
      if (!this.rootObjectBox || this.rootObjectBox.repObject != aRootObject) {
        if (this.rootObjectBox) {
          try {
            this.box.removeChild(this.rootObjectBox);
          } catch (exc) {
            InspectorUI._log("this.box.removeChild(this.rootObjectBox) FAILS " +
              this.box + " must not contain " + this.rootObjectBox);
          }
        }

        this.highlightedObjectBox = null;
        this.selectedObjectBox = null;
        this.rootObjectBox = this.view.createObjectBox(aObject, true);
        this.box.appendChild(this.rootObjectBox);
      }
      return this.rootObjectBox;
    }

    let parentNode = this.view.getParentObject(aObject);
    let parentObjectBox = this.createObjectBoxes(parentNode, aRootObject);

    if (!parentObjectBox)
      return null;

    let parentChildBox = this.getChildObjectBox(parentObjectBox);

    if (!parentChildBox)
      return null;

    let childObjectBox = this.findChildObjectBox(parentChildBox, aObject);

    return childObjectBox ? childObjectBox
      : this.populateChildBox(aObject, parentChildBox);
  },

  





  findObjectBox: function IOBox_findObjectBox(aObject)
  {
    if (!aObject)
      return null;

    if (aObject == this.rootObject)
      return this.rootObjectBox;

    let parentNode = this.view.getParentObject(aObject);
    let parentObjectBox = this.findObjectBox(parentNode);
    if (!parentObjectBox)
      return null;

    let parentChildBox = this.getChildObjectBox(parentObjectBox);
    if (!parentChildBox)
      return null;

    return this.findChildObjectBox(parentChildBox, aObject);
  },

  getAncestorByClass: function IOBox_getAncestorByClass(node, className)
  {
    for (let parent = node; parent; parent = parent.parentNode) {
      if (this.view.hasClass(parent, className))
        return parent;
    }

    return null;
  },

  


  populateChildBox: function IOBox_populateChildBox(repObject, nodeChildBox)
  {
    if (!repObject)
      return null;

    let parentObjectBox = this.getAncestorByClass(nodeChildBox, "nodeBox");

    if (parentObjectBox.populated)
      return this.findChildObjectBox(nodeChildBox, repObject);

    let lastSiblingBox = this.getChildObjectBox(nodeChildBox);
    let siblingBox = nodeChildBox.firstChild;
    let targetBox = null;
    let view = this.view;
    let targetSibling = null;
    let parentNode = view.getParentObject(repObject);

    for (let i = 0; 1; ++i) {
      targetSibling = view.getChildObject(parentNode, i, targetSibling);
      if (!targetSibling)
        break;

      
      if (lastSiblingBox && lastSiblingBox.repObject == targetSibling)
        lastSiblingBox = null;

      if (!siblingBox || siblingBox.repObject != targetSibling) {
        let newBox = view.createObjectBox(targetSibling);
        if (newBox) {
          if (lastSiblingBox)
            nodeChildBox.insertBefore(newBox, lastSiblingBox);
          else
            nodeChildBox.appendChild(newBox);
        }

        siblingBox = newBox;
      }

      if (targetSibling == repObject)
        targetBox = siblingBox;

      if (siblingBox && siblingBox.repObject == targetSibling)
        siblingBox = siblingBox.nextSibling;
    }

    if (targetBox)
      parentObjectBox.populated = true;

    return targetBox;
  },

  





  getParentObjectBox: function IOBox_getParentObjectBox(aObjectBox)
  {
    let parent = aObjectBox.parentNode ? aObjectBox.parentNode.parentNode : null;
    return parent && parent.repObject ? parent : null;
  },

  





  getChildObjectBox: function IOBox_getChildObjectBox(aObjectBox)
  {
    return aObjectBox.querySelector(".nodeChildBox");
  },

  








  findChildObjectBox: function IOBox_findChildObjectBox(aParentNodeBox, aRepObject)
  {
    let childBox = aParentNodeBox.firstChild;
    while (childBox) {
      if (childBox.repObject == aRepObject)
        return childBox;
      childBox = childBox.nextSibling;
    }
    return null; 
  },

  





  isInExistingRoot: function IOBox_isInExistingRoot(aNode)
  {
    let parentNode = aNode;
    while (parentNode && parentNode != this.rootObject) {
      parentNode = this.view.getParentObject(parentNode);
    }
    return parentNode == this.rootObject;
  },

  





  getRootNode: function IOBox_getRootNode(aNode)
  {
    let node = aNode;
    let tmpNode;
    while ((tmpNode = this.view.getParentObject(node)))
      node = tmpNode;

    return node;
  },
};
