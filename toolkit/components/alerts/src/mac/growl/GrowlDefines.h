



#ifndef _GROWLDEFINES_H
#define _GROWLDEFINES_H

#ifdef __OBJC__
#define XSTR(x) (@x)
#define STRING NSString *
#else
#define XSTR CFSTR
#define STRING CFStringRef
#endif












#pragma mark UserInfo Keys for Registration






















#define GROWL_APP_NAME					XSTR("ApplicationName")










#define GROWL_APP_ICON					XSTR("ApplicationIcon")







#define GROWL_NOTIFICATIONS_DEFAULT		XSTR("DefaultNotifications")






#define GROWL_NOTIFICATIONS_ALL			XSTR("AllNotifications")





#define GROWL_TICKET_VERSION			XSTR("TicketVersion")

#pragma mark UserInfo Keys for Notifications


















#define GROWL_NOTIFICATION_NAME			XSTR("NotificationName")





#define GROWL_NOTIFICATION_TITLE		XSTR("NotificationTitle")






#define GROWL_NOTIFICATION_DESCRIPTION  	XSTR("NotificationDescription")






#define GROWL_NOTIFICATION_ICON			XSTR("NotificationIcon")







#define GROWL_NOTIFICATION_APP_ICON		XSTR("NotificationAppIcon")






#define GROWL_NOTIFICATION_PRIORITY		XSTR("NotificationPriority")





#define GROWL_NOTIFICATION_STICKY		XSTR("NotificationSticky")












#define GROWL_NOTIFICATION_CLICK_CONTEXT			XSTR("NotificationClickContext")









#define GROWL_DISPLAY_PLUGIN				XSTR("NotificationDisplayPlugin")










#define GROWL_NOTIFICATION_IDENTIFIER	XSTR("GrowlNotificationIdentifier")








#define GROWL_APP_PID					XSTR("ApplicationPID")


#pragma mark Notifications





























#define GROWL_APP_REGISTRATION			XSTR("GrowlApplicationRegistrationNotification")






#define GROWL_APP_REGISTRATION_CONF		XSTR("GrowlApplicationRegistrationConfirmationNotification")
























#define GROWL_NOTIFICATION				XSTR("GrowlNotification")





#define GROWL_SHUTDOWN					XSTR("GrowlShutdown")





#define GROWL_PING						XSTR("Honey, Mind Taking Out The Trash")




#define GROWL_PONG						XSTR("What Do You Want From Me, Woman")







#define GROWL_IS_READY					XSTR("Lend Me Some Sugar; I Am Your Neighbor!")







#define GROWL_NOTIFICATION_CLICKED		XSTR("GrowlClicked!")
#define GROWL_NOTIFICATION_TIMED_OUT	XSTR("GrowlTimedOut!")









#define GROWL_KEY_CLICKED_CONTEXT		XSTR("ClickedContext")








#define GROWL_REG_DICT_EXTENSION		XSTR("growlRegDict")

#endif 
