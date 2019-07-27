


























package ch.boye.httpclientandroidlib.params;

import java.nio.charset.CodingErrorAction;

import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;









@Deprecated
public final class HttpProtocolParams implements CoreProtocolPNames {

    private HttpProtocolParams() {
        super();
    }

    






    public static String getHttpElementCharset(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        String charset = (String) params.getParameter
            (CoreProtocolPNames.HTTP_ELEMENT_CHARSET);
        if (charset == null) {
            charset = HTTP.DEF_PROTOCOL_CHARSET.name();
        }
        return charset;
    }

    





    public static void setHttpElementCharset(final HttpParams params, final String charset) {
        Args.notNull(params, "HTTP parameters");
        params.setParameter(CoreProtocolPNames.HTTP_ELEMENT_CHARSET, charset);
    }

    






    public static String getContentCharset(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        String charset = (String) params.getParameter
            (CoreProtocolPNames.HTTP_CONTENT_CHARSET);
        if (charset == null) {
            charset = HTTP.DEF_CONTENT_CHARSET.name();
        }
        return charset;
    }

    





    public static void setContentCharset(final HttpParams params, final String charset) {
        Args.notNull(params, "HTTP parameters");
        params.setParameter(CoreProtocolPNames.HTTP_CONTENT_CHARSET, charset);
    }

    






    public static ProtocolVersion getVersion(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        final Object param = params.getParameter
            (CoreProtocolPNames.PROTOCOL_VERSION);
        if (param == null) {
            return HttpVersion.HTTP_1_1;
        }
        return (ProtocolVersion)param;
    }

    





    public static void setVersion(final HttpParams params, final ProtocolVersion version) {
        Args.notNull(params, "HTTP parameters");
        params.setParameter(CoreProtocolPNames.PROTOCOL_VERSION, version);
    }

    






    public static String getUserAgent(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return (String) params.getParameter(CoreProtocolPNames.USER_AGENT);
    }

    





    public static void setUserAgent(final HttpParams params, final String useragent) {
        Args.notNull(params, "HTTP parameters");
        params.setParameter(CoreProtocolPNames.USER_AGENT, useragent);
    }

    






    public static boolean useExpectContinue(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getBooleanParameter(CoreProtocolPNames.USE_EXPECT_CONTINUE, false);
    }

    





    public static void setUseExpectContinue(final HttpParams params, final boolean b) {
        Args.notNull(params, "HTTP parameters");
        params.setBooleanParameter(CoreProtocolPNames.USE_EXPECT_CONTINUE, b);
    }

    






    public static CodingErrorAction getMalformedInputAction(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        final Object param = params.getParameter(CoreProtocolPNames.HTTP_MALFORMED_INPUT_ACTION);
        if (param == null) {
            
            return CodingErrorAction.REPORT;
        }
        return (CodingErrorAction) param;
    }

    






    public static void setMalformedInputAction(final HttpParams params, final CodingErrorAction action) {
        Args.notNull(params, "HTTP parameters");
        params.setParameter(CoreProtocolPNames.HTTP_MALFORMED_INPUT_ACTION, action);
    }

    






    public static CodingErrorAction getUnmappableInputAction(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        final Object param = params.getParameter(CoreProtocolPNames.HTTP_UNMAPPABLE_INPUT_ACTION);
        if (param == null) {
            
            return CodingErrorAction.REPORT;
        }
        return (CodingErrorAction) param;
    }

    






    public static void setUnmappableInputAction(final HttpParams params, final CodingErrorAction action) {
        Args.notNull(params, "HTTP parameters");
        params.setParameter(CoreProtocolPNames.HTTP_UNMAPPABLE_INPUT_ACTION, action);
    }

}
