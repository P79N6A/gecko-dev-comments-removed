


























package ch.boye.httpclientandroidlib;

import java.io.IOException;







public interface HttpServerConnection extends HttpConnection {

    









    HttpRequest receiveRequestHeader()
        throws HttpException, IOException;

    






    void receiveRequestEntity(HttpEntityEnclosingRequest request)
        throws HttpException, IOException;

    





    void sendResponseHeader(HttpResponse response)
        throws HttpException, IOException;

    





    void sendResponseEntity(HttpResponse response)
        throws HttpException, IOException;

    



    void flush()
        throws IOException;

}
