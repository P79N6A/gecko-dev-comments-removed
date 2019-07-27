


























package ch.boye.httpclientandroidlib.message;

import java.util.NoSuchElementException;

import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.TokenIterator;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;









@NotThreadSafe
public class BasicTokenIterator implements TokenIterator {

    
    
    
    public final static String HTTP_SEPARATORS = " ,;=()<>@:\\\"/[]?{}\t";


    
    protected final HeaderIterator headerIt;

    




    protected String currentHeader;

    



    protected String currentToken;

    



    protected int searchPos;


    




    public BasicTokenIterator(final HeaderIterator headerIterator) {
        super();
        this.headerIt = Args.notNull(headerIterator, "Header iterator");
        this.searchPos = findNext(-1);
    }


    
    public boolean hasNext() {
        return (this.currentToken != null);
    }


    







    public String nextToken()
        throws NoSuchElementException, ParseException {

        if (this.currentToken == null) {
            throw new NoSuchElementException("Iteration already finished.");
        }

        final String result = this.currentToken;
        
        this.searchPos = findNext(this.searchPos);

        return result;
    }


    








    public final Object next()
        throws NoSuchElementException, ParseException {
        return nextToken();
    }


    




    public final void remove()
        throws UnsupportedOperationException {

        throw new UnsupportedOperationException
            ("Removing tokens is not supported.");
    }


    















    protected int findNext(final int pos) throws ParseException {
        int from = pos;
        if (from < 0) {
            
            if (!this.headerIt.hasNext()) {
                return -1;
            }
            this.currentHeader = this.headerIt.nextHeader().getValue();
            from = 0;
        } else {
            
            from = findTokenSeparator(from);
        }

        final int start = findTokenStart(from);
        if (start < 0) {
            this.currentToken = null;
            return -1; 
        }

        final int end = findTokenEnd(start);
        this.currentToken = createToken(this.currentHeader, start, end);
        return end;
    }


    



















    protected String createToken(final String value, final int start, final int end) {
        return value.substring(start, end);
    }


    









    protected int findTokenStart(final int pos) {
        int from = Args.notNegative(pos, "Search position");
        boolean found = false;
        while (!found && (this.currentHeader != null)) {

            final int to = this.currentHeader.length();
            while (!found && (from < to)) {

                final char ch = this.currentHeader.charAt(from);
                if (isTokenSeparator(ch) || isWhitespace(ch)) {
                    
                    from++;
                } else if (isTokenChar(this.currentHeader.charAt(from))) {
                    
                    found = true;
                } else {
                    throw new ParseException
                        ("Invalid character before token (pos " + from +
                         "): " + this.currentHeader);
                }
            }
            if (!found) {
                if (this.headerIt.hasNext()) {
                    this.currentHeader = this.headerIt.nextHeader().getValue();
                    from = 0;
                } else {
                    this.currentHeader = null;
                }
            }
        } 

        return found ? from : -1;
    }


    
















    protected int findTokenSeparator(final int pos) {
        int from = Args.notNegative(pos, "Search position");
        boolean found = false;
        final int to = this.currentHeader.length();
        while (!found && (from < to)) {
            final char ch = this.currentHeader.charAt(from);
            if (isTokenSeparator(ch)) {
                found = true;
            } else if (isWhitespace(ch)) {
                from++;
            } else if (isTokenChar(ch)) {
                throw new ParseException
                    ("Tokens without separator (pos " + from +
                     "): " + this.currentHeader);
            } else {
                throw new ParseException
                    ("Invalid character after token (pos " + from +
                     "): " + this.currentHeader);
            }
        }

        return from;
    }


    










    protected int findTokenEnd(final int from) {
        Args.notNegative(from, "Search position");
        final int to = this.currentHeader.length();
        int end = from+1;
        while ((end < to) && isTokenChar(this.currentHeader.charAt(end))) {
            end++;
        }

        return end;
    }


    










    protected boolean isTokenSeparator(final char ch) {
        return (ch == ',');
    }


    










    protected boolean isWhitespace(final char ch) {

        
        
        return ((ch == '\t') || Character.isSpaceChar(ch));
    }


    











    protected boolean isTokenChar(final char ch) {

        
        if (Character.isLetterOrDigit(ch)) {
            return true;
        }

        
        if (Character.isISOControl(ch)) {
            return false;
        }

        
        if (isHttpSeparator(ch)) {
            return false;
        }

        
        
        
        
        
        
        return true;
    }


    









    protected boolean isHttpSeparator(final char ch) {
        return (HTTP_SEPARATORS.indexOf(ch) >= 0);
    }


} 

