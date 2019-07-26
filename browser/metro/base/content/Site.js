



'use strict';





function Site(aLink) {
  if(!aLink.url) {
    throw Cr.NS_ERROR_INVALID_ARG;
  }
  this._link = aLink;
}

Site.prototype = {
  icon: '',
  get url() {
    return this._link.url;
  },
  get title() {
    
    return this._link.title || this._link.url;
  },
  get label() {
    
    return this.title;
  },
  get pinned() {
    return NewTabUtils.pinnedLinks.isPinned(this);
  },
  get contextActions() {
    return [
      'delete', 
      this.pinned ? 'unpin' : 'pin'
    ];
  },
  blocked: false,
  get attributeValues() {
    return {
      value: this.url,
      label: this.title,
      pinned: this.pinned ? true : undefined,
      selected: this.selected,
      customColor: this.color,
      customImage: this.backgroundImage,
      iconURI: this.icon,
      "data-contextactions": this.contextActions.join(',')
    };
  },
  applyToTileNode: function(aNode) {
    
    
    let attrs = this.attributeValues;
    for (let key in attrs) {
      if (undefined === attrs[key]) {
        aNode.removeAttribute(key);
      } else {
        aNode.setAttribute(key, attrs[key]);
      }
    }
    
    if (aNode.refresh) {
      
      aNode.refresh();
    } else {
      
    }
  }
};
