


























package ch.boye.httpclientandroidlib;

import ch.boye.httpclientandroidlib.params.HttpParams;

























@SuppressWarnings("deprecation")
public interface HttpMessage {

    


    ProtocolVersion getProtocolVersion();

    






    boolean containsHeader(String name);

    







    Header[] getHeaders(String name);

    










    Header getFirstHeader(String name);

    









    Header getLastHeader(String name);

    





    Header[] getAllHeaders();

    





    void addHeader(Header header);

    






    void addHeader(String name, String value);

    





    void setHeader(Header header);

    






    void setHeader(String name, String value);

    




    void setHeaders(Header[] headers);

    




    void removeHeader(Header header);

    




    void removeHeaders(String name);

    





    HeaderIterator headerIterator();

    








    HeaderIterator headerIterator(String name);

    






    @Deprecated
    HttpParams getParams();

    






    @Deprecated
    void setParams(HttpParams params);

}
