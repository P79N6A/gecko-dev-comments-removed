

























package ch.boye.httpclientandroidlib.client.entity;

import java.io.UnsupportedEncodingException;
import java.util.List;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;

import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.client.utils.URLEncodedUtils;
import ch.boye.httpclientandroidlib.entity.StringEntity;
import ch.boye.httpclientandroidlib.protocol.HTTP;







@NotThreadSafe 
public class UrlEncodedFormEntity extends StringEntity {

    







    public UrlEncodedFormEntity (
        final List <? extends NameValuePair> parameters,
        final String encoding) throws UnsupportedEncodingException {
        super(URLEncodedUtils.format(parameters, encoding), encoding);
        setContentType(URLEncodedUtils.CONTENT_TYPE + HTTP.CHARSET_PARAM +
                (encoding != null ? encoding : HTTP.DEFAULT_CONTENT_CHARSET));
    }

    






    public UrlEncodedFormEntity (
        final List <? extends NameValuePair> parameters) throws UnsupportedEncodingException {
        this(parameters, HTTP.DEFAULT_CONTENT_CHARSET);
    }

}
