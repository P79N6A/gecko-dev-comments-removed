


























package ch.boye.httpclientandroidlib.message;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;









@NotThreadSafe
public class HeaderGroup implements Cloneable, Serializable {

    private static final long serialVersionUID = 2608834160639271617L;

    
    private final List<Header> headers;

    


    public HeaderGroup() {
        this.headers = new ArrayList<Header>(16);
    }

    


    public void clear() {
        headers.clear();
    }

    





    public void addHeader(final Header header) {
        if (header == null) {
            return;
        }
        headers.add(header);
    }

    




    public void removeHeader(final Header header) {
        if (header == null) {
            return;
        }
        headers.remove(header);
    }

    






    public void updateHeader(final Header header) {
        if (header == null) {
            return;
        }
        
        
        
        for (int i = 0; i < this.headers.size(); i++) {
            final Header current = this.headers.get(i);
            if (current.getName().equalsIgnoreCase(header.getName())) {
                this.headers.set(i, header);
                return;
            }
        }
        this.headers.add(header);
    }

    






    public void setHeaders(final Header[] headers) {
        clear();
        if (headers == null) {
            return;
        }
        Collections.addAll(this.headers, headers);
    }

    










    public Header getCondensedHeader(final String name) {
        final Header[] hdrs = getHeaders(name);

        if (hdrs.length == 0) {
            return null;
        } else if (hdrs.length == 1) {
            return hdrs[0];
        } else {
            final CharArrayBuffer valueBuffer = new CharArrayBuffer(128);
            valueBuffer.append(hdrs[0].getValue());
            for (int i = 1; i < hdrs.length; i++) {
                valueBuffer.append(", ");
                valueBuffer.append(hdrs[i].getValue());
            }

            return new BasicHeader(name.toLowerCase(Locale.ENGLISH), valueBuffer.toString());
        }
    }

    









    public Header[] getHeaders(final String name) {
        final List<Header> headersFound = new ArrayList<Header>();
        
        
        
        for (int i = 0; i < this.headers.size(); i++) {
            final Header header = this.headers.get(i);
            if (header.getName().equalsIgnoreCase(name)) {
                headersFound.add(header);
            }
        }

        return headersFound.toArray(new Header[headersFound.size()]);
    }

    







    public Header getFirstHeader(final String name) {
        
        
        
        for (int i = 0; i < this.headers.size(); i++) {
            final Header header = this.headers.get(i);
            if (header.getName().equalsIgnoreCase(name)) {
                return header;
            }
        }
        return null;
    }

    







    public Header getLastHeader(final String name) {
        
        for (int i = headers.size() - 1; i >= 0; i--) {
            final Header header = headers.get(i);
            if (header.getName().equalsIgnoreCase(name)) {
                return header;
            }
        }

        return null;
    }

    




    public Header[] getAllHeaders() {
        return headers.toArray(new Header[headers.size()]);
    }

    








    public boolean containsHeader(final String name) {
        
        
        
        for (int i = 0; i < this.headers.size(); i++) {
            final Header header = this.headers.get(i);
            if (header.getName().equalsIgnoreCase(name)) {
                return true;
            }
        }

        return false;
    }

    






    public HeaderIterator iterator() {
        return new BasicListHeaderIterator(this.headers, null);
    }

    









    public HeaderIterator iterator(final String name) {
        return new BasicListHeaderIterator(this.headers, name);
    }

    






    public HeaderGroup copy() {
        final HeaderGroup clone = new HeaderGroup();
        clone.headers.addAll(this.headers);
        return clone;
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

    @Override
    public String toString() {
        return this.headers.toString();
    }

}
