






















package nu.validator.htmlparser.impl;

import nu.validator.htmlparser.annotation.Local;
import nu.validator.htmlparser.annotation.NsUri;

final class StackNode<T> {
    final int group;

    final @Local String name;

    final @Local String popName;

    final @NsUri String ns;

    final T node;

    final boolean scoping;

    final boolean special;

    final boolean fosterParenting;
    
    private int refcount = 1;

    









    StackNode(int group, final @NsUri String ns, final @Local String name, final T node,
            final boolean scoping, final boolean special,
            final boolean fosterParenting, final @Local String popName) {
        this.group = group;
        this.name = name;
        this.popName = popName;
        this.ns = ns;
        this.node = node;
        this.scoping = scoping;
        this.special = special;
        this.fosterParenting = fosterParenting;
        this.refcount = 1;
        Portability.retainLocal(name);
        Portability.retainLocal(popName);
        Portability.retainElement(node);
        
    }

    




    StackNode(final @NsUri String ns, ElementName elementName, final T node) {
        this.group = elementName.group;
        this.name = elementName.name;
        this.popName = elementName.name;
        this.ns = ns;
        this.node = node;
        this.scoping = elementName.scoping;
        this.special = elementName.special;
        this.fosterParenting = elementName.fosterParenting;
        this.refcount = 1;
        Portability.retainLocal(name);
        Portability.retainLocal(popName);
        Portability.retainElement(node);
        
    }

    StackNode(final @NsUri String ns, ElementName elementName, final T node, @Local String popName) {
        this.group = elementName.group;
        this.name = elementName.name;
        this.popName = popName;
        this.ns = ns;
        this.node = node;
        this.scoping = elementName.scoping;
        this.special = elementName.special;
        this.fosterParenting = elementName.fosterParenting;
        this.refcount = 1;
        Portability.retainLocal(name);
        Portability.retainLocal(popName);
        Portability.retainElement(node);
        
    }

    StackNode(final @NsUri String ns, ElementName elementName, final T node, @Local String popName, boolean scoping) {
        this.group = elementName.group;
        this.name = elementName.name;
        this.popName = popName;
        this.ns = ns;
        this.node = node;
        this.scoping = scoping;
        this.special = false;
        this.fosterParenting = false;
        this.refcount = 1;
        Portability.retainLocal(name);
        Portability.retainLocal(popName);
        Portability.retainElement(node);
        
    }
    
    @SuppressWarnings("unused") private void destructor() {
        Portability.releaseLocal(name);
        Portability.releaseLocal(popName);
        Portability.releaseElement(node);
        
    }
    
    
    


    @Override public @Local String toString() {
        return name;
    }
    
    
    public void retain() {   
        refcount++;
    }
    
    public void release() {
        refcount--;
        if (refcount == 0) {
            Portability.delete(this);
        }
    }
}
