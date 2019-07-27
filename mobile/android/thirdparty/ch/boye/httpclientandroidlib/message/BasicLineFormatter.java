


























package ch.boye.httpclientandroidlib.message;

import ch.boye.httpclientandroidlib.FormattedHeader;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;













@Immutable
public class BasicLineFormatter implements LineFormatter {

    







    @Deprecated
    public final static BasicLineFormatter DEFAULT = new BasicLineFormatter();

    public final static BasicLineFormatter INSTANCE = new BasicLineFormatter();

    public BasicLineFormatter() {
        super();
    }

    







    protected CharArrayBuffer initBuffer(final CharArrayBuffer charBuffer) {
        CharArrayBuffer buffer = charBuffer;
        if (buffer != null) {
            buffer.clear();
        } else {
            buffer = new CharArrayBuffer(64);
        }
        return buffer;
    }


    









    public static
        String formatProtocolVersion(final ProtocolVersion version,
                                     final LineFormatter formatter) {
        return (formatter != null ? formatter : BasicLineFormatter.INSTANCE)
                .appendProtocolVersion(null, version).toString();
    }


    
    public CharArrayBuffer appendProtocolVersion(final CharArrayBuffer buffer,
                                                 final ProtocolVersion version) {
        Args.notNull(version, "Protocol version");
        
        CharArrayBuffer result = buffer;
        final int len = estimateProtocolVersionLen(version);
        if (result == null) {
            result = new CharArrayBuffer(len);
        } else {
            result.ensureCapacity(len);
        }

        result.append(version.getProtocol());
        result.append('/');
        result.append(Integer.toString(version.getMajor()));
        result.append('.');
        result.append(Integer.toString(version.getMinor()));

        return result;
    }


    








    protected int estimateProtocolVersionLen(final ProtocolVersion version) {
        return version.getProtocol().length() + 4; 
    }


    









    public static String formatRequestLine(final RequestLine reqline,
                                           final LineFormatter formatter) {
        return (formatter != null ? formatter : BasicLineFormatter.INSTANCE)
                .formatRequestLine(null, reqline).toString();
    }


    
    public CharArrayBuffer formatRequestLine(final CharArrayBuffer buffer,
                                             final RequestLine reqline) {
        Args.notNull(reqline, "Request line");
        final CharArrayBuffer result = initBuffer(buffer);
        doFormatRequestLine(result, reqline);

        return result;
    }


    







    protected void doFormatRequestLine(final CharArrayBuffer buffer,
                                       final RequestLine reqline) {
        final String method = reqline.getMethod();
        final String uri    = reqline.getUri();

        
        final int len = method.length() + 1 + uri.length() + 1 +
            estimateProtocolVersionLen(reqline.getProtocolVersion());
        buffer.ensureCapacity(len);

        buffer.append(method);
        buffer.append(' ');
        buffer.append(uri);
        buffer.append(' ');
        appendProtocolVersion(buffer, reqline.getProtocolVersion());
    }



    









    public static String formatStatusLine(final StatusLine statline,
                                          final LineFormatter formatter) {
        return (formatter != null ? formatter : BasicLineFormatter.INSTANCE)
                .formatStatusLine(null, statline).toString();
    }


    
    public CharArrayBuffer formatStatusLine(final CharArrayBuffer buffer,
                                            final StatusLine statline) {
        Args.notNull(statline, "Status line");
        final CharArrayBuffer result = initBuffer(buffer);
        doFormatStatusLine(result, statline);

        return result;
    }


    







    protected void doFormatStatusLine(final CharArrayBuffer buffer,
                                      final StatusLine statline) {

        int len = estimateProtocolVersionLen(statline.getProtocolVersion())
            + 1 + 3 + 1; 
        final String reason = statline.getReasonPhrase();
        if (reason != null) {
            len += reason.length();
        }
        buffer.ensureCapacity(len);

        appendProtocolVersion(buffer, statline.getProtocolVersion());
        buffer.append(' ');
        buffer.append(Integer.toString(statline.getStatusCode()));
        buffer.append(' '); 
        if (reason != null) {
            buffer.append(reason);
        }
    }


    









    public static String formatHeader(final Header header,
                                      final LineFormatter formatter) {
        return (formatter != null ? formatter : BasicLineFormatter.INSTANCE)
                .formatHeader(null, header).toString();
    }


    
    public CharArrayBuffer formatHeader(final CharArrayBuffer buffer,
                                        final Header header) {
        Args.notNull(header, "Header");
        final CharArrayBuffer result;

        if (header instanceof FormattedHeader) {
            
            result = ((FormattedHeader)header).getBuffer();
        } else {
            result = initBuffer(buffer);
            doFormatHeader(result, header);
        }
        return result;

    } 


    







    protected void doFormatHeader(final CharArrayBuffer buffer,
                                  final Header header) {
        final String name = header.getName();
        final String value = header.getValue();

        int len = name.length() + 2;
        if (value != null) {
            len += value.length();
        }
        buffer.ensureCapacity(len);

        buffer.append(name);
        buffer.append(": ");
        if (value != null) {
            buffer.append(value);
        }
    }


} 
