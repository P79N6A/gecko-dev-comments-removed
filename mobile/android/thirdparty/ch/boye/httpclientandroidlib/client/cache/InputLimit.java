

























package ch.boye.httpclientandroidlib.client.cache;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;







@NotThreadSafe 
public class InputLimit {

    private final long value;
    private boolean reached;

    




    public InputLimit(final long value) {
        super();
        this.value = value;
        this.reached = false;
    }

    



    public long getValue() {
        return this.value;
    }

    


    public void reached() {
        this.reached = true;
    }

    


    public boolean isReached() {
        return this.reached;
    }

}
