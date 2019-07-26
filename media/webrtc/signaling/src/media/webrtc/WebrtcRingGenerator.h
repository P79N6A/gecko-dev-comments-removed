






































#ifndef WebrtcRINGGENERATOR_H
#define WebrtcRINGGENERATOR_H

#ifndef _USE_CPVE

#include <CSFAudioTermination.h>
#include "common_types.h"

namespace CSF {

	class WebrtcRingGenerator : public webrtc::InStream
	{
	public:
		WebrtcRingGenerator( RingMode mode, bool once );

		
		int Read( void *buf, int len );
		void SetScaleFactor(int scaleFactor); 

	private:
		RingMode mode;
		bool once;
		int currentStep;
		int timeRemaining;	
		bool done;
		int scaleFactor;

		int generateTone( short *buf, int numSamples );
		void applyScaleFactor( short *buf, int numSamples );
	};

} 

#endif
#endif 
