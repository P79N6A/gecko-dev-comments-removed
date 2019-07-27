

























package ch.boye.httpclientandroidlib.client.entity;

import java.io.UnsupportedEncodingException;
import java.nio.charset.Charset;
import java.util.List;

import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.client.utils.URLEncodedUtils;
import ch.boye.httpclientandroidlib.entity.ContentType;
import ch.boye.httpclientandroidlib.entity.StringEntity;
import ch.boye.httpclientandroidlib.protocol.HTTP;







@NotThreadSafe 
public class UrlEncodedFormEntity extends StringEntity {

    







    public UrlEncodedFormEntity (
        final List <? extends NameValuePair> parameters,
        final String charset) throws UnsupportedEncodingException {
        super(URLEncodedUtils.format(parameters,
                charset != null ? charset : HTTP.DEF_CONTENT_CHARSET.name()),
                ContentType.create(URLEncodedUtils.CONTENT_TYPE, charset));
    }

    








    public UrlEncodedFormEntity (
        final Iterable <? extends NameValuePair> parameters,
        final Charset charset) {
        super(URLEncodedUtils.format(parameters,
                charset != null ? charset : HTTP.DEF_CONTENT_CHARSET),
                ContentType.create(URLEncodedUtils.CONTENT_TYPE, charset));
    }

    






    public UrlEncodedFormEntity (
        final List <? extends NameValuePair> parameters) throws UnsupportedEncodingException {
        this(parameters, (Charset) null);
    }

    







    public UrlEncodedFormEntity (
        final Iterable <? extends NameValuePair> parameters) {
        this(parameters, null);
    }

}
