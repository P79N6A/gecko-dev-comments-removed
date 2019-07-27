


























package ch.boye.httpclientandroidlib.conn.routing;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class BasicRouteDirector implements HttpRouteDirector {

    










    public int nextStep(final RouteInfo plan, final RouteInfo fact) {
        Args.notNull(plan, "Planned route");

        int step = UNREACHABLE;

        if ((fact == null) || (fact.getHopCount() < 1)) {
            step = firstStep(plan);
        } else if (plan.getHopCount() > 1) {
            step = proxiedStep(plan, fact);
        } else {
            step = directStep(plan, fact);
        }

        return step;

    } 


    






    protected int firstStep(final RouteInfo plan) {

        return (plan.getHopCount() > 1) ?
            CONNECT_PROXY : CONNECT_TARGET;
    }


    








    protected int directStep(final RouteInfo plan, final RouteInfo fact) {

        if (fact.getHopCount() > 1) {
            return UNREACHABLE;
        }
        if (!plan.getTargetHost().equals(fact.getTargetHost()))
         {
            return UNREACHABLE;
        
        
        
        
        }

        
        
        if (plan.isSecure() != fact.isSecure()) {
            return UNREACHABLE;
        }

        
        if ((plan.getLocalAddress() != null) &&
            !plan.getLocalAddress().equals(fact.getLocalAddress())
            ) {
            return UNREACHABLE;
        }

        return COMPLETE;
    }


    








    protected int proxiedStep(final RouteInfo plan, final RouteInfo fact) {

        if (fact.getHopCount() <= 1) {
            return UNREACHABLE;
        }
        if (!plan.getTargetHost().equals(fact.getTargetHost())) {
            return UNREACHABLE;
        }
        final int phc = plan.getHopCount();
        final int fhc = fact.getHopCount();
        if (phc < fhc) {
            return UNREACHABLE;
        }

        for (int i=0; i<fhc-1; i++) {
            if (!plan.getHopTarget(i).equals(fact.getHopTarget(i))) {
                return UNREACHABLE;
            }
        }
        
        if (phc > fhc)
         {
            return TUNNEL_PROXY; 
        }

        
        if ((fact.isTunnelled() && !plan.isTunnelled()) ||
            (fact.isLayered()   && !plan.isLayered())) {
            return UNREACHABLE;
        }

        if (plan.isTunnelled() && !fact.isTunnelled()) {
            return TUNNEL_TARGET;
        }
        if (plan.isLayered() && !fact.isLayered()) {
            return LAYER_PROTOCOL;
        }

        
        
        
        if (plan.isSecure() != fact.isSecure()) {
            return UNREACHABLE;
        }

        return COMPLETE;
    }

}
