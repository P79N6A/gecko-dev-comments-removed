


























package ch.boye.httpclientandroidlib.conn.routing;

import java.net.InetAddress;

import ch.boye.httpclientandroidlib.HttpHost;






public interface RouteInfo {

    







    public enum TunnelType { PLAIN, TUNNELLED }

    












    public enum LayerType  { PLAIN, LAYERED }

    




    HttpHost getTargetHost();

    





    InetAddress getLocalAddress();

    






    int getHopCount();

    















    HttpHost getHopTarget(int hop);

    





    HttpHost getProxyHost();

    





    TunnelType getTunnelType();

    







    boolean isTunnelled();

    






    LayerType getLayerType();

    







    boolean isLayered();

    





    boolean isSecure();

}
