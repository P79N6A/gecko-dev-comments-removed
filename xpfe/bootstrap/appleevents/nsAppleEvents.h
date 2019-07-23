




































#define MOZILLA_FIVE





	





#define		kSpyGlass_CmdBase			5000		// Base value for SpyGlass commands
#define		kURLSuite_CmdBase			5100		// Base value for URL commands




enum	{
	kRequired_aedtResID	= 128,			
	kCore_aedtResID,					
	kMisc_aedtResID,					
	kPowerPlant_aedtResID,				
	kURLSuite_aedtResID,				
	kSpyGlass_aedtResID					
};



	













enum	{
	AE_OpenURL = kSpyGlass_CmdBase 

#ifndef MOZILLA_FIVE
,	AE_RegisterViewer				
,	AE_UnregisterViewer				
,	AE_ShowFile						
,	AE_ParseAnchor					
,	AE_RegisterURLEcho				
,	AE_UnregisterURLEcho			
,	AE_SpyActivate					
,	AE_SpyListWindows				
,	AE_GetWindowInfo				
,	AE_RegisterWinClose				
,	AE_UnregisterWinClose			
,	AE_RegisterProtocol				
,	AE_UnregisterProtocol			
,	AE_CancelProgress				
,	AE_FindURL						
#endif 
};



enum	{

#ifdef MOZILLA_FIVE
	AE_GetURL = kURLSuite_CmdBase	
,	AE_DoJavascript					
#else
	AE_GetWD = kURLSuite_CmdBase	
,	AE_OpenBookmark					
,	AE_ReadHelpFile					
,	AE_Go							
,	AE_OpenProfileManager			
,	AE_GetURL						
,	AE_OpenAddressBook				
,	AE_OpenComponent				
,	AE_GetActiveProfile				
,	AE_HandleCommand				
,	AE_GetProfileImportData			
,	AE_OpenGuestMode				
#endif 
};





#define AE_GetOption			4016	// GetOption
#define AE_SetOption			4017	// SetOption
#define AE_ListOptions			4018	// ListOptions




#define AE_ViewDocFile			4019	// ViewDocFile
#define AE_BeginProgress		4020	// Begin progress
#define AE_SetProgressRange		4021	// Set progress range
#define AE_MakingProgress		4022	// Making progress
#define AE_EndProgress			4023	// End progress
#define AE_QueryViewer			4024	// Query viewer
#define AE_URLEcho				4026	// URL echo
#define AE_WindowClose			4027	// WindowClose
















#define AE_www_suite 					'MOSS'

#define AE_www_doJavaScript				'jscr'  // Execute a javascript string

#ifndef MOZILLA_FIVE
#define AE_www_workingURL				'wurl'	// Get working URL

#define AE_www_go	 					'gogo'	// keyDirectObject HWIN, direction 'dire'

#define AE_www_go_direction 			'dire'	// directions
#define AE_www_go_again 				'agai'	// keyDirectObject HWIN 
#define AE_www_go_home					'home'	// keyDirectObject HWIN 
#define AE_www_super_reload				'srld'	// keyDirectObject HWIN 

#define AE_www_openBookmark				'book'	// Open Bookmark file
#define AE_www_openAddressBook			'addr'	// Open Address Book

#define AE_www_ReadHelpFile  			'help'	// keyDirectObject is the file
#define AE_www_ReadHelpFileID 			'idid'	// Help file id. If none, use "DEFAULT"
#define AE_www_ReadHelpFileSearchText 	'sear' // Search text, no default

#define AE_www_ProfileManager			'prfl'	//obsolete
#define AE_www_GuestMode				'gues'  // Open in guest (roaming only) mode for kiosks


#define AE_www_openComponent			'cpnt'

#define AE_www_comp_navigator			'navg'
#define AE_www_comp_inbox				'inbx'
#define AE_www_comp_collabra			'colb'
#define AE_www_comp_composer			'cpsr'
#define AE_www_comp_conference			'conf'
#define AE_www_comp_calendar			'cald'
#define AE_www_comp_ibmHostOnDemand		'ibmh'
#define AE_www_comp_netcaster			'netc'


#define AE_www_handleCommand			'hcmd'


#define AE_www_getActiveProfile			'upro'


#define AE_www_getImportData			'Impt'
#endif 


#define AE_www_typeWindow				'HWIN'

#define AE_www_typeWindowURL			'curl'	// Property: current URL
#define AE_www_typeWindowID				'wiid'	// unique ID
#define AE_www_typeWindowBusy 			'busy'	// Are we busy

#define AE_www_typeApplicationAlert 	'ALAP'
#define AE_www_typeKioskMode			'KOSK'	// Kiosk mode












#define AE_url_suite				'GURL'


#define AE_url_getURL 				'GURL'	// keyDirectObject typeChar URL, 

#define AE_url_getURLdestination 	'dest'	
#define AE_url_getURLrefererer 		'refe'
#define AE_url_getURLname			'name'	// window name























































































#define AE_spy_receive_suite		'WWW!'
#define AE_spy_send_suite			'WWW?'






#define AE_spy_openURL		'OURL'	// typeChar OpenURL

#define AE_spy_openURL_flag 'FLGS'	// typeLongInteger flags
#define AE_spy_openURL_wind	'WIND'	// typeLongInteger windowID

#if 0 

#define AE_spy_openURL_into 'INTO'	// typeFSS into
#define AE_spy_openURL_post	'POST'	// typeWildCard post data
#define AE_spy_openURL_mime 'MIME'	// typeChar MIME type
#define AE_spy_openURL_prog 'PROG'	// typePSN Progress app


#define AE_spy_showFile		'SHWF'	// typeAlias file spec
#define AE_spy_showFile_mime 'MIME'	// typeChar MIME type
#define AE_spy_showFile_win	'WIND'	// WindowID
#define AE_spy_showFile_url 'URL '	// URL

#define AE_spy_parse		'PRSA'	// typeChar main URL
#define AE_spy_parse_rel	'RELA'	// typeChar relative URL


#define AE_spy_CancelProgress 'CNCL' // typeLongInteger transactionID
#define AE_spy_CancelProgress_win 'WIND' // typeLongInteger windowID


#define AE_spy_findURL		'FURL'	// typeFSS file spec. Returns the URL of the file




#define AE_spy_activate				'ACTV'	// typeLong window ID
#define AE_spy_activate_flags		'FLGS'	// typeLong unused flags

#define AE_spy_listwindows			'LSTW'	// no arguments

#define AE_spy_getwindowinfo		'WNFO'	// typeLong window






#define AE_spy_registerURLecho		'RGUE'	// typeApplSignature application

#define AE_spy_unregisterURLecho	'UNRU'	// typeApplSignature application


#define AE_spy_registerViewer		'RGVW'	//  typeSign	Application
#define AE_spy_registerViewer_mime	'MIME'	// typeChar		Mime type
#define AE_spy_registerViewer_flag	'MTHD'	// typeLongInteger Flags
#define AE_spy_registerViewer_ftyp	'FTYP'	// file type

#define AE_spy_unregisterViewer		'UNRV'	// typeApplSignature application
#define AE_spy_unregisterViewer_mime 'MIME'	// MIME type


#define AE_spy_register_protocol		'RGPR'	// typeApplSignature application
#define AE_spy_register_protocol_pro 	'PROT'	// typeChar protocol

#define AE_spy_unregister_protocol		'UNRP'	// typeApplSignature application
#define AE_spy_register_protocol_pro 	'PROT'	// typeChar protocol


#define AE_spy_registerWinClose		'RGWC'	// typeApplSignature application
#define AE_spy_registerWinClose_win	'WIND'// typeLong window

#define AE_spy_unregisterWinClose		'UNRC'	// typeApplSignature application
#define AE_spy_unregisterWinClose_win	'WIND'// typeLong window



#define AE_spy_setOption			'SOPT'	// typeChar option name
#define AE_spy_setOption_value		'OPTV'	// type depends upon the option

#define AE_spy_getOption			'GOPT'	// typeChar option name

#define AE_spy_listOptions			'LOPT'	// no arguments





#define AE_spy_viewDocFile			'VDOC'	// typeAlias	fileSpec
#define AE_spy_viewDocFile_url		'URL '	// typeChar	url
#define AE_spy_viewDocFile_mime		'MIME'	// typeChar mimeType
#define AE_spy_viewDocFile_wind		'WIND'	// typeLongInteger Window ID

#define AE_spy_beginProgress		'PRBG'	// typeLongInteger windowID
#define AE_spy_beginProgress_msg	'PMSG'	// typeChar message

#define AE_spy_setProgressRange		'PRSR'	// typeLongInteger transactionID
#define AE_spy_setProgressRange_max 'MAXV'	// typeLongInteger max

#define AE_spy_makingProgress		'PRMK'	// typeLongInteger transactionID
#define AE_spy_makingProgress_msg	'PMSG'	// typeChar message
#define AE_spy_makingProgress_curr	'CURR'	// typeLongInteger current data size

#define AE_spy_endProgress			'PREN'	// typeLongInteger transactionID

#define AE_spy_queryViewer			'QVWR'	// typeChar url
#define AE_spy_queryViewer_mime		'MIME'	// typeChar MIME type

#define AE_spy_URLecho				'URLE'	// typeChar url
#define AE_spy_URLecho_mime			'MIME'	// typeChar MIME type
#define AE_spy_URLecho_win			'WIND'	// typeLongInt windowID
#define AE_spy_URLecho_referer		'RFRR'	// typeChar referer

#define AE_spy_winClosed			'WNDC'	// typeLong windowID
#define AE_spy_winClosedExiting		'EXIT'	// typeBoolean are we quitting?

#endif 



	










#define cEuMailfolder     'euMF'  // Class: 			folder for mailboxes and mail folders
#define pEuTopLevel       'euTL'  // Property boolean:  is top-level of Eudora Folder?
#define pEuFSS            'euFS'  // Property alias:  	FSS for file

#define cEuMailbox        'euMB'  /* mailbox */
#define pEuMailboxType    'euMT'  /* in, out, trash, ... */
#define pEuWasteSpace     'euWS'  /* space wasted in mailbox */
#define pEuNeededSpace    'euNS'  /* space needed by messages in mailbox */
#define pEuTOCFSS         'eTFS'  /* FSS for toc file (pEuFSS is for mailbox) */

#define cEuNotify         'eNot'  /* applications to notify */
                                  

#define cEuMessage        'euMS'  /* message */
#define pEuPriority       'euPY'  /* priority */
#define pEuStatus         'euST'  /* message status */
#define pEuSender         'euSe'  /* sender */
#define pEuDate           'euDa'  /* date */
#define pEuSize           'euSi'  /* size */
#define pEuSubject        'euSu'  /* subject */
#define pEuOutgoing       'euOu'  /* outgoing? */
#define pEuSignature      'eSig'  /* signature? */
#define pEuWrap           'eWrp'  /* wrap? */
#define pEuFakeTabs       'eTab'  /* fake tabs? */
#define pEuKeepCopy       'eCpy'  /* keep copy? */
#define pEuHqxText        'eXTX'  /* HQX -> TEXT? */
#define pEuMayQP          'eMQP'  /* may use quoted-printable? */
#define pEuAttachType     'eATy'  /* attachment type; 0 double, 1 single, 2 hqx, 3 uuencode */
#define pEuShowAll        'eBla'  /* show all headers */
#define pEuTableId        'eTbl'  /* resource id of table */
#define pEuBody           'eBod'  /* resource id of table */
#define pEuSelectedText   'eStx'  /* the text selected now */
#define pEuWillFetch      'eWFh'  /* is on list to fetch next time */
#define pEuWillDelete     'eWDl'  /* is on list to delete next time */
#define pEuReturnReceipt  'eRRR'  /* return receipt requested */
#define pEuLabel          'eLbl'  /* label index */

#define cEuField          'euFd'  /* field in message */

#define cEu822Address     'e822'  /* RFC 822 address */

#define cEuTEInWin        'EuWT'  /* the teh of a window */
#define cEuWTEText        'eWTT'  /* text from the teh of a window */

#define cEuPreference     'ePrf'  /* a preference string */

#define kEudoraSuite      'CSOm'  /* Eudora suite */
#define keyEuNotify       'eNot'  /* Notify of new mail */
#define kEuNotify         keyEuNotify
#define kEuInstallNotify  'nIns'  /* install a notification */
#define kEuRemoveNotify   'nRem'  /* remove a notification */
#define keyEuWhatHappened 'eWHp'  /* what happened */
#define keyEuMessList     'eMLs'  /* Message list */

#define eMailArrive       'wArv'  /* mail has arrived */
#define eMailSent         'wSnt'  /* mail has been sent */
#define eWillConnect      'wWCn'  /* will connect */
#define eHasConnected     'wHCn'  /* has connected */

#define kEuReply          'eRep'  /* Reply */
#define keyEuToWhom       'eRWh'  /* Reply to anyone in particular? */
#define keyEuReplyAll     'eRAl'  /* Reply to all? */
#define keyEuIncludeSelf  'eSlf'  /* Include self? */
#define keyEuQuoteText    'eQTx'  /* Quote original message text? */

#define kEuForward        'eFwd'  /* Forward */

#define kEuRedirect       'eRdr'  /* Redirect */

#define kEuSalvage        'eSav'  /* Salvage a message */

#define kEuAttach         'eAtc'  /* Attach a document */
#define keyEuDocumentList 'eDcl'  /* List of dox to attach */

#define kEuQueue          'eQue'  /* Queue a message */
#define keyEuWhen         'eWhn'  /* When to send message */

#define kEuUnQueue        'eUnQ'  /* Unqueue a message */

#define kEuConnect        'eCon'  /* Connect (send/queue) */
#define keyEuSend         'eSen'
#define keyEuCheck        'eChk'
#define keyEuOnIdle       'eIdl'  /* wait until Eudora is idle? */

#define kEuNewAttach      'euAD'  /* attach document, new style */
#define keyEuToWhat       'euMS'  /* attach to what message? */

#define typeVDId          'VDId'  /* vref & dirid */

#define kIn               IN
#define kOut              OUT
#define kTrash            TRASH
#define KRegular          0



























