


























package ch.boye.httpclientandroidlib.params;

import java.nio.charset.Charset;
import java.nio.charset.CodingErrorAction;

import ch.boye.httpclientandroidlib.config.ConnectionConfig;
import ch.boye.httpclientandroidlib.config.MessageConstraints;
import ch.boye.httpclientandroidlib.config.SocketConfig;






@Deprecated
public final class HttpParamConfig {

    private HttpParamConfig() {
    }

    public static SocketConfig getSocketConfig(final HttpParams params) {
        return SocketConfig.custom()
                .setSoTimeout(params.getIntParameter(CoreConnectionPNames.SO_TIMEOUT, 0))
                .setSoReuseAddress(params.getBooleanParameter(CoreConnectionPNames.SO_REUSEADDR, false))
                .setSoKeepAlive(params.getBooleanParameter(CoreConnectionPNames.SO_KEEPALIVE, false))
                .setSoLinger(params.getIntParameter(CoreConnectionPNames.SO_LINGER, -1))
                .setTcpNoDelay(params.getBooleanParameter(CoreConnectionPNames.TCP_NODELAY, true))
                .build();
    }

    public static MessageConstraints getMessageConstraints(final HttpParams params) {
        return MessageConstraints.custom()
                .setMaxHeaderCount(params.getIntParameter(CoreConnectionPNames.MAX_HEADER_COUNT, -1))
                .setMaxLineLength(params.getIntParameter(CoreConnectionPNames.MAX_LINE_LENGTH, -1))
                .build();
    }

    public static ConnectionConfig getConnectionConfig(final HttpParams params) {
        final MessageConstraints messageConstraints = getMessageConstraints(params);
        final String csname = (String) params.getParameter(CoreProtocolPNames.HTTP_ELEMENT_CHARSET);
        return ConnectionConfig.custom()
                .setCharset(csname != null ? Charset.forName(csname) : null)
                .setMalformedInputAction((CodingErrorAction)
                        params.getParameter(CoreProtocolPNames.HTTP_MALFORMED_INPUT_ACTION))
                .setMalformedInputAction((CodingErrorAction)
                        params.getParameter(CoreProtocolPNames.HTTP_UNMAPPABLE_INPUT_ACTION))
                .setMessageConstraints(messageConstraints)
                .build();
    }

}
