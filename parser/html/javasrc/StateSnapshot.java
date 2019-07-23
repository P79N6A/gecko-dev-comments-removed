





















package nu.validator.htmlparser.impl;


public class StateSnapshot<T> {

    




    StateSnapshot(StackNode<T>[] stack,
            StackNode<T>[] listOfActiveFormattingElements, T formPointer) {
        this.stack = stack;
        this.listOfActiveFormattingElements = listOfActiveFormattingElements;
        this.formPointer = formPointer;
    }

    final StackNode<T>[] stack;

    final StackNode<T>[] listOfActiveFormattingElements;

    final T formPointer;
    
    @SuppressWarnings("unused") private void destructor() {
        for (int i = 0; i < stack.length; i++) {
            stack[i].release();
        }
        Portability.releaseArray(stack);
        for (int i = 0; i < listOfActiveFormattingElements.length; i++) {
            if (listOfActiveFormattingElements[i] != null) {
                listOfActiveFormattingElements[i].release();                
            }
        }
        Portability.releaseArray(listOfActiveFormattingElements);
        Portability.retainElement(formPointer);
    }
}
