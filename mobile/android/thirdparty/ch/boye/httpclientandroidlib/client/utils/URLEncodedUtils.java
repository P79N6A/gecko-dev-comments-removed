


























package ch.boye.httpclientandroidlib.client.utils;

import java.io.IOException;
import java.net.URI;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.BitSet;
import java.util.Collections;
import java.util.List;
import java.util.Scanner;

import ch.boye.httpclientandroidlib.Consts;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.entity.ContentType;
import ch.boye.httpclientandroidlib.message.BasicHeaderValueParser;
import ch.boye.httpclientandroidlib.message.BasicNameValuePair;
import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;
import ch.boye.httpclientandroidlib.util.EntityUtils;






@Immutable
public class URLEncodedUtils {

    


    public static final String CONTENT_TYPE = "application/x-www-form-urlencoded";

    private static final char QP_SEP_A = '&';
    private static final char QP_SEP_S = ';';
    private static final String NAME_VALUE_SEPARATOR = "=";

    














    public static List <NameValuePair> parse(final URI uri, final String charset) {
        final String query = uri.getRawQuery();
        if (query != null && query.length() > 0) {
            final List<NameValuePair> result = new ArrayList<NameValuePair>();
            final Scanner scanner = new Scanner(query);
            parse(result, scanner, QP_SEP_PATTERN, charset);
            return result;
        }
        return Collections.emptyList();
    }

    











    public static List <NameValuePair> parse(
            final HttpEntity entity) throws IOException {
        final ContentType contentType = ContentType.get(entity);
        if (contentType != null && contentType.getMimeType().equalsIgnoreCase(CONTENT_TYPE)) {
            final String content = EntityUtils.toString(entity, Consts.ASCII);
            if (content != null && content.length() > 0) {
                Charset charset = contentType.getCharset();
                if (charset == null) {
                    charset = HTTP.DEF_CONTENT_CHARSET;
                }
                return parse(content, charset, QP_SEPS);
            }
        }
        return Collections.emptyList();
    }

    



    public static boolean isEncoded(final HttpEntity entity) {
        final Header h = entity.getContentType();
        if (h != null) {
            final HeaderElement[] elems = h.getElements();
            if (elems.length > 0) {
                final String contentType = elems[0].getName();
                return contentType.equalsIgnoreCase(CONTENT_TYPE);
            }
        }
        return false;
    }

    












    public static void parse(
            final List <NameValuePair> parameters,
            final Scanner scanner,
            final String charset) {
        parse(parameters, scanner, QP_SEP_PATTERN, charset);
    }

    















    public static void parse(
            final List <NameValuePair> parameters,
            final Scanner scanner,
            final String parameterSepartorPattern,
            final String charset) {
        scanner.useDelimiter(parameterSepartorPattern);
        while (scanner.hasNext()) {
            String name = null;
            String value = null;
            final String token = scanner.next();
            final int i = token.indexOf(NAME_VALUE_SEPARATOR);
            if (i != -1) {
                name = decodeFormFields(token.substring(0, i).trim(), charset);
                value = decodeFormFields(token.substring(i + 1).trim(), charset);
            } else {
                name = decodeFormFields(token.trim(), charset);
            }
            parameters.add(new BasicNameValuePair(name, value));
        }
    }

    


    private static final char[] QP_SEPS = new char[] { QP_SEP_A, QP_SEP_S };

    


    private static final String QP_SEP_PATTERN = "[" + new String(QP_SEPS) + "]";

    











    public static List<NameValuePair> parse(final String s, final Charset charset) {
        return parse(s, charset, QP_SEPS);
    }

    













    public static List<NameValuePair> parse(final String s, final Charset charset, final char... parameterSeparator) {
        if (s == null) {
            return Collections.emptyList();
        }
        final BasicHeaderValueParser parser = BasicHeaderValueParser.INSTANCE;
        final CharArrayBuffer buffer = new CharArrayBuffer(s.length());
        buffer.append(s);
        final ParserCursor cursor = new ParserCursor(0, buffer.length());
        final List<NameValuePair> list = new ArrayList<NameValuePair>();
        while (!cursor.atEnd()) {
            final NameValuePair nvp = parser.parseNameValuePair(buffer, cursor, parameterSeparator);
            if (nvp.getName().length() > 0) {
                list.add(new BasicNameValuePair(
                        decodeFormFields(nvp.getName(), charset),
                        decodeFormFields(nvp.getValue(), charset)));
            }
        }
        return list;
    }

    







    public static String format(
            final List <? extends NameValuePair> parameters,
            final String charset) {
        return format(parameters, QP_SEP_A, charset);
    }

    










    public static String format(
            final List <? extends NameValuePair> parameters,
            final char parameterSeparator,
            final String charset) {
        final StringBuilder result = new StringBuilder();
        for (final NameValuePair parameter : parameters) {
            final String encodedName = encodeFormFields(parameter.getName(), charset);
            final String encodedValue = encodeFormFields(parameter.getValue(), charset);
            if (result.length() > 0) {
                result.append(parameterSeparator);
            }
            result.append(encodedName);
            if (encodedValue != null) {
                result.append(NAME_VALUE_SEPARATOR);
                result.append(encodedValue);
            }
        }
        return result.toString();
    }

    









    public static String format(
            final Iterable<? extends NameValuePair> parameters,
            final Charset charset) {
        return format(parameters, QP_SEP_A, charset);
    }

    










    public static String format(
            final Iterable<? extends NameValuePair> parameters,
            final char parameterSeparator,
            final Charset charset) {
        final StringBuilder result = new StringBuilder();
        for (final NameValuePair parameter : parameters) {
            final String encodedName = encodeFormFields(parameter.getName(), charset);
            final String encodedValue = encodeFormFields(parameter.getValue(), charset);
            if (result.length() > 0) {
                result.append(parameterSeparator);
            }
            result.append(encodedName);
            if (encodedValue != null) {
                result.append(NAME_VALUE_SEPARATOR);
                result.append(encodedValue);
            }
        }
        return result.toString();
    }

    





    private static final BitSet UNRESERVED   = new BitSet(256);
    




    private static final BitSet PUNCT        = new BitSet(256);
    

    private static final BitSet USERINFO     = new BitSet(256);
    

    private static final BitSet PATHSAFE     = new BitSet(256);
    

    private static final BitSet URIC     = new BitSet(256);

    







    private static final BitSet RESERVED     = new BitSet(256);


    



    private static final BitSet URLENCODER   = new BitSet(256);

    static {
        
        
        for (int i = 'a'; i <= 'z'; i++) {
            UNRESERVED.set(i);
        }
        for (int i = 'A'; i <= 'Z'; i++) {
            UNRESERVED.set(i);
        }
        
        for (int i = '0'; i <= '9'; i++) {
            UNRESERVED.set(i);
        }
        UNRESERVED.set('_'); 
        UNRESERVED.set('-');
        UNRESERVED.set('.');
        UNRESERVED.set('*');
        URLENCODER.or(UNRESERVED); 
        UNRESERVED.set('!');
        UNRESERVED.set('~');
        UNRESERVED.set('\'');
        UNRESERVED.set('(');
        UNRESERVED.set(')');
        
        PUNCT.set(',');
        PUNCT.set(';');
        PUNCT.set(':');
        PUNCT.set('$');
        PUNCT.set('&');
        PUNCT.set('+');
        PUNCT.set('=');
        
        USERINFO.or(UNRESERVED);
        USERINFO.or(PUNCT);

        
        PATHSAFE.or(UNRESERVED);
        PATHSAFE.set('/'); 
        PATHSAFE.set(';'); 
        PATHSAFE.set(':'); 
        PATHSAFE.set('@');
        PATHSAFE.set('&');
        PATHSAFE.set('=');
        PATHSAFE.set('+');
        PATHSAFE.set('$');
        PATHSAFE.set(',');

        RESERVED.set(';');
        RESERVED.set('/');
        RESERVED.set('?');
        RESERVED.set(':');
        RESERVED.set('@');
        RESERVED.set('&');
        RESERVED.set('=');
        RESERVED.set('+');
        RESERVED.set('$');
        RESERVED.set(',');
        RESERVED.set('['); 
        RESERVED.set(']'); 

        URIC.or(RESERVED);
        URIC.or(UNRESERVED);
    }

    private static final int RADIX = 16;

    private static String urlEncode(
            final String content,
            final Charset charset,
            final BitSet safechars,
            final boolean blankAsPlus) {
        if (content == null) {
            return null;
        }
        final StringBuilder buf = new StringBuilder();
        final ByteBuffer bb = charset.encode(content);
        while (bb.hasRemaining()) {
            final int b = bb.get() & 0xff;
            if (safechars.get(b)) {
                buf.append((char) b);
            } else if (blankAsPlus && b == ' ') {
                buf.append('+');
            } else {
                buf.append("%");
                final char hex1 = Character.toUpperCase(Character.forDigit((b >> 4) & 0xF, RADIX));
                final char hex2 = Character.toUpperCase(Character.forDigit(b & 0xF, RADIX));
                buf.append(hex1);
                buf.append(hex2);
            }
        }
        return buf.toString();
    }

    







    private static String urlDecode(
            final String content,
            final Charset charset,
            final boolean plusAsBlank) {
        if (content == null) {
            return null;
        }
        final ByteBuffer bb = ByteBuffer.allocate(content.length());
        final CharBuffer cb = CharBuffer.wrap(content);
        while (cb.hasRemaining()) {
            final char c = cb.get();
            if (c == '%' && cb.remaining() >= 2) {
                final char uc = cb.get();
                final char lc = cb.get();
                final int u = Character.digit(uc, 16);
                final int l = Character.digit(lc, 16);
                if (u != -1 && l != -1) {
                    bb.put((byte) ((u << 4) + l));
                } else {
                    bb.put((byte) '%');
                    bb.put((byte) uc);
                    bb.put((byte) lc);
                }
            } else if (plusAsBlank && c == '+') {
                bb.put((byte) ' ');
            } else {
                bb.put((byte) c);
            }
        }
        bb.flip();
        return charset.decode(bb).toString();
    }

    






    private static String decodeFormFields (final String content, final String charset) {
        if (content == null) {
            return null;
        }
        return urlDecode(content, charset != null ? Charset.forName(charset) : Consts.UTF_8, true);
    }

    






    private static String decodeFormFields (final String content, final Charset charset) {
        if (content == null) {
            return null;
        }
        return urlDecode(content, charset != null ? charset : Consts.UTF_8, true);
    }

    










    private static String encodeFormFields(final String content, final String charset) {
        if (content == null) {
            return null;
        }
        return urlEncode(content, charset != null ? Charset.forName(charset) : Consts.UTF_8, URLENCODER, true);
    }

    










    private static String encodeFormFields (final String content, final Charset charset) {
        if (content == null) {
            return null;
        }
        return urlEncode(content, charset != null ? charset : Consts.UTF_8, URLENCODER, true);
    }

    








    static String encUserInfo(final String content, final Charset charset) {
        return urlEncode(content, charset, USERINFO, false);
    }

    








    static String encUric(final String content, final Charset charset) {
        return urlEncode(content, charset, URIC, false);
    }

    








    static String encPath(final String content, final Charset charset) {
        return urlEncode(content, charset, PATHSAFE, false);
    }

}
