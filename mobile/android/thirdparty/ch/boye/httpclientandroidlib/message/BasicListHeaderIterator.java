


























package ch.boye.httpclientandroidlib.message;

import java.util.List;
import java.util.NoSuchElementException;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;







@NotThreadSafe
public class BasicListHeaderIterator implements HeaderIterator {

    



    protected final List<Header> allHeaders;


    



    protected int currentIndex;


    



    protected int lastIndex;


    



    protected String headerName;



    






    public BasicListHeaderIterator(final List<Header> headers, final String name) {
        super();
        this.allHeaders = Args.notNull(headers, "Header list");
        this.headerName = name;
        this.currentIndex = findNext(-1);
        this.lastIndex = -1;
    }


    








    protected int findNext(final int pos) {
        int from = pos;
        if (from < -1) {
            return -1;
        }

        final int to = this.allHeaders.size()-1;
        boolean found = false;
        while (!found && (from < to)) {
            from++;
            found = filterHeader(from);
        }
        return found ? from : -1;
    }


    







    protected boolean filterHeader(final int index) {
        if (this.headerName == null) {
            return true;
        }

        
        final String name = (this.allHeaders.get(index)).getName();

        return this.headerName.equalsIgnoreCase(name);
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

        this.lastIndex    = current;
        this.currentIndex = findNext(current);

        return this.allHeaders.get(current);
    }


    







    public final Object next()
        throws NoSuchElementException {
        return nextHeader();
    }


    


    public void remove()
        throws UnsupportedOperationException {
        Asserts.check(this.lastIndex >= 0, "No header to remove");
        this.allHeaders.remove(this.lastIndex);
        this.lastIndex = -1;
        this.currentIndex--; 
    }
}
