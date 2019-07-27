

var Aria = {
	Trees: new Array(), 
	isEnabled: function(inNode){
		
		if(inNode.getAttribute('aria-enabled') && inNode.getAttribute('aria-enabled').toLowerCase()=='false') return false;
		else return true;
	},
	isExpanded: function(inNode){
		if(inNode.getAttribute('aria-expanded') && inNode.getAttribute('aria-expanded').toLowerCase()=='false') return false;
		else return true;
	},
	isTreeItem: function(inNode){
		if(inNode.getAttribute('role') && inNode.getAttribute('role').toLowerCase()=='treeitem') return true;
		else return false;
	}
};

Aria.Tree = Class.create();
Aria.Tree.prototype = {
	initialize: function(inNode){
		if(!$(inNode) && console.error) console.error('Error from aria.js: Aria.Tree instance initialized with invalid element, '+ inNode);
		this.el = $(inNode);
		this.index = Aria.Trees.length; 
		this.strActiveDescendant = this.el.getAttribute('aria-activedescendant');
		this.strDefaultActiveDescendant = 'tree'+this.index+'_item0'; 
		if(!$(this.strActiveDescendant)) this.strActiveDescendant = this.strDefaultActiveDescendant; 
		this.setActiveDescendant($(this.strActiveDescendant));
		
		
		Event.observe(this.el, 'click', this.handleClick.bindAsEventListener(this));
		Event.observe(this.el, 'keydown', this.handleKeyPress.bindAsEventListener(this)); 
		
	},
	getActiveDescendant: function(inNode){
		if(inNode){ 
			var el = $(inNode);
			while(el != this.el){
				if(Aria.isTreeItem(el)) break; 
				el = el.parentNode;
			}
			if(el == this.el) {
				this.setActiveDescendant(); 
			} else {
				this.setActiveDescendant(el);
				return el;
			}
		} else {
			return $(this.el.getAttribute('aria-activedescendant'));
		}
	},
	getNextTreeItem: function(inNode){
		var el = $(inNode);
		var originalElm = $(inNode);
		while(!Aria.isTreeItem(el) || el == originalElm){
			if(Aria.isExpanded(el) && el.down()){ 
				var elements = el.getElementsByTagName('*');
				for(var i=0, c=elements.length; i<c; i++){
					if(Aria.isTreeItem(elements[i])) return elements[i];
				}
			}
			if(el.next()){
				el = el.next();
			} else {
				while(!el.parentNode.next() && el.parentNode != this.el){
					el = el.parentNode;
				}
				if(el.parentNode == this.el) return originalElm; 
				else el = el.parentNode.next();
			}
		}
		return el;
	},
	getPreviousTreeItem: function(inNode){
		var el = $(inNode);
		var originalElm = $(inNode);
		while(!Aria.isTreeItem(el) || el == originalElm){
			if(el.previous()){
				el = el.previous();
				
				if (el.down() && Aria.isExpanded(el)){
					el = el.down();
					while (el.next() || (el.down() && Aria.isExpanded(el))){
						if (el.next()) el = el.next();
						else el = el.down();
					}
				}
			} else {
				if(el.parentNode == this.el) return originalElm; 
				el = el.parentNode;
			}
		}
		if(el == this.el) return originalElm; 
		return el;
	},
	handleClick: function(inEvent){
		var target = inEvent.target; 
		var el = this.getActiveDescendant(target);
		if(target.className.indexOf('expander')>-1){ 
			this.toggleExpanded(el); 
			Event.stop(inEvent); 
		}
	},
	handleKeyPress: function(inEvent){
		switch(inEvent.keyCode){
			
			
			
			
			case Event.KEY_LEFT:     this.keyLeft();  break;
			case Event.KEY_UP:       this.keyUp();    break;
			case Event.KEY_RIGHT:    this.keyRight(); break;
			case Event.KEY_DOWN:     this.keyDown();  break;
			default: 
				
				return;
		}
		Event.stop(inEvent);
	},
	keyLeft: function(){
		var el = this.activeDescendant;
		if(Aria.isExpanded(el)){
			el.setAttribute('aria-expanded','false');
			this.setActiveDescendant(this.activeDescendant);
		}
	},
	keyUp: function(){
		var el = this.activeDescendant;
		this.setActiveDescendant(this.getPreviousTreeItem(el));
	},
	keyRight: function(){
		var el = this.activeDescendant;
		if(!Aria.isExpanded(el)){
			el.setAttribute('aria-expanded','true');
			this.setActiveDescendant(this.activeDescendant);
		}
	},
	keyDown: function(){
		var el = this.activeDescendant;
		this.setActiveDescendant(this.getNextTreeItem(el));
	},
	setActiveDescendant: function(inNode){
		Element.removeClassName(this.activeDescendant,'activedescendant')
		if($(inNode)) this.activeDescendant = $(inNode);
		else this.activeDescendant = $(this.strDefaultActiveDescendant);
		Element.addClassName(this.activeDescendant,'activedescendant')
		this.strActiveDescendant = this.activeDescendant.id;
		this.el.setAttribute('aria-activedescendant', this.activeDescendant.id);
	},
	toggleExpanded: function(inNode){
		var el = $(inNode);
		if(Aria.isExpanded(el)){
			el.setAttribute('aria-expanded','false');
		} else {
			el.setAttribute('aria-expanded','true');	
		}
		this.setActiveDescendant(el);
	}
};
