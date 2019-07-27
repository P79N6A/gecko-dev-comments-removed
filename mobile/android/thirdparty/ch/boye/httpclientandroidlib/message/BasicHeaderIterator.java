


























package ch.boye.httpclientandroidlib.message;

import java.util.NoSuchElementException;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;






@NotThreadSafe
public class BasicHeaderIterator implements HeaderIterator {

    





    protected final Header[] allHeaders;


    



    protected int currentIndex;


    



    protected String headerName;



    






    public BasicHeaderIterator(final Header[] headers, final String name) {
        super();
        this.allHeaders = Args.notNull(headers, "Header array");
        this.headerName = name;
        this.currentIndex = findNext(-1);
    }


    








    protected int findNext(final int pos) {
        int from = pos;
        if (from < -1) {
            return -1;
        }

        final int to = this.allHeaders.length-1;
        boolean found = false;
        while (!found && (from < to)) {
            from++;
            found = filterHeader(from);
        }
        return found ? from : -1;
    }


    







    protected boolean filterHeader(final int index) {
        return (this.headerName == null) ||
            this.headerName.equalsIgnoreCase(this.allHeaders[index].getName());
    }


    
    public boolean hasNext() {
        return (this.currentIndex >= 0);
    }


    






    public Header nextHeader()
        throws NoSuchElementException {

        final int current = this.currentIndex;
        if (current < 0) {
            throw new NoSuchElementException("Iteration already finished.");
        }

        this.currentIndex = findNext(current);

        return this.allHeaders[current];
    }


    







    public final Object next()
        throws NoSuchElementException {
        return nextHeader();
    }


    




    public void remove()
        throws UnsupportedOperationException {

        throw new UnsupportedOperationException
            ("Removing headers is not supported.");
    }
}
