






































#ifndef WebrtcTONEGENERATOR_H
#define WebrtcTONEGENERATOR_H

#ifndef _USE_CPVE

#include <CSFAudioTermination.h>
#include "common_types.h"

#define MAX_TONEGENS		4
#define MAX_CADENCES		4
#define MAX_REPEATCNTS		4
#define TGN_INFINITE_REPEAT	65535L

#define TG_DESCRIPTOR_CADENCE_MASK  0xF     // descriptor cadence mask
#define TG_MAX_CADENCES				MAX_CADENCES * sizeof(CADENCE_DURATION) / sizeof(unsigned short)
#define TG_MAX_REPEATCNTS			MAX_REPEATCNTS * sizeof (REPEAT_COUNT_TABLE) / sizeof(short)

namespace CSF {

	typedef struct {
		short OnDuration;
		short OffDuration;
	} CADENCE_DURATION;

	typedef struct {
		short FilterMemory;
		short FilterCoef;
	} FREQ_COEF_TABLE;

	typedef struct {
		short RepeatCount;
	} REPEAT_COUNT_TABLE;

	typedef struct {
		CADENCE_DURATION	Cadence[MAX_CADENCES];   
		FREQ_COEF_TABLE		Coefmem[MAX_TONEGENS];   
		REPEAT_COUNT_TABLE	rCount[MAX_REPEATCNTS];
		unsigned short		RepeatCount;
		unsigned short		Descriptor;
	} TONE_TABLE_TYPE, *PTONE_TABLE_TYPE;

	class WebrtcToneGenerator : public webrtc::InStream
	{
	public:
		WebrtcToneGenerator( ToneType type );

		
		int Read( void *buf, int len );

	private:
		typedef struct {
			short Coef;
			short Yn_1;
			short Yn_2;
		} SINEWAVE, *PSINEWAVE;

		SINEWAVE			m_Sinewave[MAX_TONEGENS];
		unsigned long		m_SinewaveIdx;
		unsigned short		m_Cadence[TG_MAX_CADENCES];
		unsigned long		m_CadenceIdx;
		short				m_rCount[TG_MAX_REPEATCNTS];
		unsigned long		m_Sample;
		int					m_RepeatCount;
		unsigned short		m_Descriptor;
		short				m_CadenceRepeatCount;

		bool	TGNGenerateTone( short *dst, unsigned long length );
		void	ToneGen( PSINEWAVE param, short *dst, unsigned long length, unsigned long numTones );
	};

} 

#endif
#endif 
