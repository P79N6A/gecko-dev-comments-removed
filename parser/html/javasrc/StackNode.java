






















package nu.validator.htmlparser.impl;

import nu.validator.htmlparser.annotation.Inline;
import nu.validator.htmlparser.annotation.Local;
import nu.validator.htmlparser.annotation.NsUri;

final class StackNode<T> {
    final int flags;

    final @Local String name;

    final @Local String popName;

    final @NsUri String ns;

    final T node;

    
    HtmlAttributes attributes;

    private int refcount = 1;

    @Inline public int getFlags() {
        return flags;
    }

    public int getGroup() {
        return flags & ElementName.GROUP_MASK;
    }

    public boolean isScoping() {
        return (flags & ElementName.SCOPING) != 0;
    }

    public boolean isSpecial() {
        return (flags & ElementName.SPECIAL) != 0;
    }

    public boolean isScopingOrSpecial() {
        return (flags & (ElementName.SCOPING | ElementName.SPECIAL)) != 0;
    }

    public boolean isFosterParenting() {
        return (flags & ElementName.FOSTER_PARENTING) != 0;
    }

    









    StackNode(int flags, final @NsUri String ns, final @Local String name,
            final T node, final @Local String popName, HtmlAttributes attributes) {
        this.flags = flags;
        this.name = name;
        this.popName = popName;
        this.ns = ns;
        this.node = node;
        this.attributes = attributes;
        this.refcount = 1;
        Portability.retainLocal(name);
        Portability.retainLocal(popName);
        Portability.retainElement(node);
        
    }

    




    StackNode(final @NsUri String ns, ElementName elementName, final T node) {
        this.flags = elementName.getFlags();
        this.name = elementName.name;
        this.popName = elementName.name;
        this.ns = ns;
        this.node = node;
        this.attributes = null;
        this.refcount = 1;
        Portability.retainLocal(name);
        Portability.retainLocal(popName);
        Portability.retainElement(node);
        
    }

    StackNode(final @NsUri String ns, ElementName elementName, final T node,
            HtmlAttributes attributes) {
        this.flags = elementName.getFlags();
        this.name = elementName.name;
        this.popName = elementName.name;
        this.ns = ns;
        this.node = node;
        this.attributes = attributes;
        this.refcount = 1;
        Portability.retainLocal(name);
        Portability.retainLocal(popName);
        Portability.retainElement(node);
        
    }

    StackNode(final @NsUri String ns, ElementName elementName, final T node,
            @Local String popName) {
        this.flags = elementName.getFlags();
        this.name = elementName.name;
        this.popName = popName;
        this.ns = ns;
        this.node = node;
        this.attributes = null;
        this.refcount = 1;
        Portability.retainLocal(name);
        Portability.retainLocal(popName);
        Portability.retainElement(node);
        
    }

    StackNode(final @NsUri String ns, ElementName elementName, final T node,
            @Local String popName, boolean scoping) {
        this.flags = (scoping ? (elementName.getFlags() | ElementName.SCOPING)
                : (elementName.getFlags() & ~ElementName.SCOPING))
                & ~(ElementName.SPECIAL | ElementName.FOSTER_PARENTING);
        this.name = elementName.name;
        this.popName = popName;
        this.ns = ns;
        this.node = node;
        this.attributes = null;
        this.refcount = 1;
        Portability.retainLocal(name);
        Portability.retainLocal(popName);
        Portability.retainElement(node);
        
    }

    @SuppressWarnings("unused") private void destructor() {
        Portability.releaseLocal(name);
        Portability.releaseLocal(popName);
        Portability.releaseElement(node);
        
        Portability.delete(attributes);
    }

    public void dropAttributes() {
        attributes = null;
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
