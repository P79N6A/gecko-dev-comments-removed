







#import "GrowlAbstractSingletonObject.h"

#define GROWL_PATHEXTENSION_TICKET	@"growlTicket"

@class GrowlApplicationTicket;

@interface GrowlTicketController: GrowlAbstractSingletonObject
{
	NSMutableDictionary *ticketsByApplicationName;
}

+ (id) sharedController;

- (NSDictionary *) allSavedTickets;

- (GrowlApplicationTicket *) ticketForApplicationName:(NSString *) appName;
- (void) addTicket:(GrowlApplicationTicket *) newTicket;
- (void) removeTicketForApplicationName:(NSString *)appName;

- (void) loadAllSavedTickets;
@end
