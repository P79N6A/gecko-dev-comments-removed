


























package ch.boye.httpclientandroidlib.message;

import java.util.NoSuchElementException;

import ch.boye.httpclientandroidlib.FormattedHeader;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HeaderElementIterator;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;






@NotThreadSafe
public class BasicHeaderElementIterator implements HeaderElementIterator {

    private final HeaderIterator headerIt;
    private final HeaderValueParser parser;

    private HeaderElement currentElement = null;
    private CharArrayBuffer buffer = null;
    private ParserCursor cursor = null;

    


    public BasicHeaderElementIterator(
            final HeaderIterator headerIterator,
            final HeaderValueParser parser) {
        this.headerIt = Args.notNull(headerIterator, "Header iterator");
        this.parser = Args.notNull(parser, "Parser");
    }


    public BasicHeaderElementIterator(final HeaderIterator headerIterator) {
        this(headerIterator, BasicHeaderValueParser.INSTANCE);
    }


    private void bufferHeaderValue() {
        this.cursor = null;
        this.buffer = null;
        while (this.headerIt.hasNext()) {
            final Header h = this.headerIt.nextHeader();
            if (h instanceof FormattedHeader) {
                this.buffer = ((FormattedHeader) h).getBuffer();
                this.cursor = new ParserCursor(0, this.buffer.length());
                this.cursor.updatePos(((FormattedHeader) h).getValuePos());
                break;
            } else {
                final String value = h.getValue();
                if (value != null) {
                    this.buffer = new CharArrayBuffer(value.length());
                    this.buffer.append(value);
                    this.cursor = new ParserCursor(0, this.buffer.length());
                    break;
                }
            }
        }
    }

    private void parseNextElement() {
        
        while (this.headerIt.hasNext() || this.cursor != null) {
            if (this.cursor == null || this.cursor.atEnd()) {
                
                bufferHeaderValue();
            }
            
            if (this.cursor != null) {
                
                while (!this.cursor.atEnd()) {
                    final HeaderElement e = this.parser.parseHeaderElement(this.buffer, this.cursor);
                    if (!(e.getName().length() == 0 && e.getValue() == null)) {
                        
                        this.currentElement = e;
                        return;
                    }
                }
                
                if (this.cursor.atEnd()) {
                    
                    this.cursor = null;
                    this.buffer = null;
                }
            }
        }
    }

    public boolean hasNext() {
        if (this.currentElement == null) {
            parseNextElement();
        }
        return this.currentElement != null;
    }

    public HeaderElement nextElement() throws NoSuchElementException {
        if (this.currentElement == null) {
            parseNextElement();
        }

        if (this.currentElement == null) {
            throw new NoSuchElementException("No more header elements available");
        }

        final HeaderElement element = this.currentElement;
        this.currentElement = null;
        return element;
    }

    public final Object next() throws NoSuchElementException {
        return nextElement();
    }

    public void remove() throws UnsupportedOperationException {
        throw new UnsupportedOperationException("Remove not supported");
    }

}
