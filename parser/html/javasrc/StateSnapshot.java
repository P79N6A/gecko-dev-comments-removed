





















package nu.validator.htmlparser.impl;

import nu.validator.htmlparser.annotation.Auto;


public class StateSnapshot<T> implements TreeBuilderState<T> {

    private final @Auto StackNode<T>[] stack;

    private final @Auto StackNode<T>[] listOfActiveFormattingElements;

    private final T formPointer;

    private final T headPointer;

    private final T deepTreeSurrogateParent;

    private final int mode;

    private final int originalMode;
    
    private final boolean framesetOk;

    private final boolean inForeign;

    private final boolean needToDropLF;

    private final boolean quirks;

    









    StateSnapshot(StackNode<T>[] stack,
            StackNode<T>[] listOfActiveFormattingElements, T formPointer, T headPointer, T deepTreeSurrogateParent, int mode, int originalMode, boolean framesetOk, boolean inForeign, boolean needToDropLF, boolean quirks) {
        this.stack = stack;
        this.listOfActiveFormattingElements = listOfActiveFormattingElements;
        this.formPointer = formPointer;
        this.headPointer = headPointer;
        this.deepTreeSurrogateParent = deepTreeSurrogateParent;
        this.mode = mode;
        this.originalMode = originalMode;
        this.framesetOk = framesetOk;
        this.inForeign = inForeign;
        this.needToDropLF = needToDropLF;
        this.quirks = quirks;
    }
    
    


    public StackNode<T>[] getStack() {
        return stack;
    }

    


    public StackNode<T>[] getListOfActiveFormattingElements() {
        return listOfActiveFormattingElements;
    }

    


    public T getFormPointer() {
        return formPointer;
    }

    




    public T getHeadPointer() {
        return headPointer;
    }

    




    public T getDeepTreeSurrogateParent() {
        return deepTreeSurrogateParent;
    }
    
    




    public int getMode() {
        return mode;
    }

    




    public int getOriginalMode() {
        return originalMode;
    }

    




    public boolean isFramesetOk() {
        return framesetOk;
    }

    




    public boolean isInForeign() {
        return inForeign;
    }

    




    public boolean isNeedToDropLF() {
        return needToDropLF;
    }

    




    public boolean isQuirks() {
        return quirks;
    }
    
    


    public int getListOfActiveFormattingElementsLength() {
        return listOfActiveFormattingElements.length;
    }

    


    public int getStackLength() {
        return stack.length;
    }

    @SuppressWarnings("unused") private void destructor() {
        for (int i = 0; i < stack.length; i++) {
            stack[i].release();
        }
        for (int i = 0; i < listOfActiveFormattingElements.length; i++) {
            if (listOfActiveFormattingElements[i] != null) {
                listOfActiveFormattingElements[i].release();                
            }
        }
        Portability.retainElement(formPointer);
    }
}
