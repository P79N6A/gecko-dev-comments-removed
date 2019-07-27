

























package ch.boye.httpclientandroidlib.conn;

import ch.boye.httpclientandroidlib.HttpHost;






public interface SchemePortResolver {

    


    int resolve(HttpHost host) throws UnsupportedSchemeException;

}
