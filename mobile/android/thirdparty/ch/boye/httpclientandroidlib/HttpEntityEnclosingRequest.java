

























package ch.boye.httpclientandroidlib;






public interface HttpEntityEnclosingRequest extends HttpRequest {

    







    boolean expectContinue();

    




    void setEntity(HttpEntity entity);

    




    HttpEntity getEntity();

}
