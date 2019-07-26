






































#ifndef _USE_CPVE

#include <string.h>
#include "WebrtcToneGenerator.h"
#include "CSFToneDefinitions.h"

namespace CSF {

static TONE_TABLE_TYPE ToneTable[] =
{
	

	
	{{{(short)TGN_INFINITE_REPEAT, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_440, TGN_COEF_440}, {TGN_YN_2_350, TGN_COEF_350}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	TGN_INFINITE_REPEAT, 0x0002},
	
	
	{{{(short)TGN_INFINITE_REPEAT, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_450, TGN_COEF_450}, {TGN_YN_2_548, TGN_COEF_548}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	TGN_INFINITE_REPEAT, 0x0002},
	
	
	{{{MILLISECONDS_TO_SAMPLES(500),  MILLISECONDS_TO_SAMPLES(500)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_480, TGN_COEF_480}, {TGN_YN_2_620, TGN_COEF_620}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	TGN_INFINITE_REPEAT, 0x0002},
	
	
	{{{MILLISECONDS_TO_SAMPLES(2000), MILLISECONDS_TO_SAMPLES(4000)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_440, TGN_COEF_440}, {TGN_YN_2_480, TGN_COEF_480}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	TGN_INFINITE_REPEAT, 0x0002},
	
    
    {{{MILLISECONDS_TO_SAMPLES(2000), MILLISECONDS_TO_SAMPLES(100)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_440, TGN_COEF_440}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{0},{0},{0},{0}},
    0x0000, 0x0002},

    
    {{{MILLISECONDS_TO_SAMPLES(100), MILLISECONDS_TO_SAMPLES(100)}, {(short)TGN_INFINITE_REPEAT, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_350, TGN_COEF_350}, {TGN_YN_2_440, TGN_COEF_440}, {TGN_YN_2_350, TGN_COEF_350}, {TGN_YN_2_440, TGN_COEF_440}},
    {{9},{0},{0},{0}},
    TGN_INFINITE_REPEAT, 0x0022},

    
    {{{MILLISECONDS_TO_SAMPLES(100), MILLISECONDS_TO_SAMPLES(100)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_350, TGN_COEF_350}, {TGN_YN_2_440, TGN_COEF_440}, {0x0000, 0x0000}, {0x0000, 0x0000}}, 
    {{0},{0},{0},{0}},
    0x0009, 0x0002},

	
	{{{MILLISECONDS_TO_SAMPLES(250),  MILLISECONDS_TO_SAMPLES(250)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_480, TGN_COEF_480}, {TGN_YN_2_620, TGN_COEF_620}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	TGN_INFINITE_REPEAT, 0x0002},
	
	
	{{{MILLISECONDS_TO_SAMPLES(400), MILLISECONDS_TO_SAMPLES(100)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_440, TGN_COEF_440}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	0x0000, 0x0001},
	
    
    {{{MILLISECONDS_TO_SAMPLES(100), MILLISECONDS_TO_SAMPLES(100)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_440, TGN_COEF_440}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}}, 
    {{0},{0},{0},{0}},
    0x0001, 0x0001},

    
    {{{MILLISECONDS_TO_SAMPLES(100), MILLISECONDS_TO_SAMPLES(100)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_440, TGN_COEF_440}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}}, 
    {{0},{0},{0},{0}},
    0x0002, 0x0001},
    
    
    {{{MILLISECONDS_TO_SAMPLES(100), MILLISECONDS_TO_SAMPLES(100)}, {MILLISECONDS_TO_SAMPLES(300), MILLISECONDS_TO_SAMPLES(100)},
    {MILLISECONDS_TO_SAMPLES(100), 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_440, TGN_COEF_440}, {TGN_YN_2_440, TGN_COEF_440}, {TGN_YN_2_440, TGN_COEF_440}, {TGN_YN_2_440, TGN_COEF_440}}, 
    {{0},{0},{0},{0}},
    0x0000, 0x0111},
    
	
	{{{MILLISECONDS_TO_SAMPLES(100), MILLISECONDS_TO_SAMPLES(150)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_500, TGN_COEF_500}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	0x0002, 0x0001},
	
    
    {{{MILLISECONDS_TO_SAMPLES(100), MILLISECONDS_TO_SAMPLES(100)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_440, TGN_COEF_440}, {TGN_YN_2_350, TGN_COEF_350}, {0x0000, 0x0000}, {0x0000, 0x0000}}, 
    {{0},{0},{0},{0}},
    0x0002, 0x0001},

    
    {{{(short)TGN_INFINITE_REPEAT,	0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_480, TGN_COEF_480}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}}, 
    {{0},{0},{0},{0}},
    TGN_INFINITE_REPEAT, 0x0001}, 
    
    
    {{{MILLISECONDS_TO_SAMPLES(500), MILLISECONDS_TO_SAMPLES(500)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_440, TGN_COEF_440}, {TGN_YN_2_480, TGN_COEF_480}, {0x0000, 0x0000}, {0x0000, 0x0000}},       
    {{0},{0},{0},{0}},
    0x0000, 0x0001},

	
	{{{MILLISECONDS_TO_SAMPLES(250),  MILLISECONDS_TO_SAMPLES(250)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_480, TGN_COEF_480}, {TGN_YN_2_620, TGN_COEF_620}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	TGN_INFINITE_REPEAT, 0x0002},
	
	
	{{{MILLISECONDS_TO_SAMPLES(300), MILLISECONDS_TO_SAMPLES(100)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_480, TGN_COEF_480}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	0x0001, 0x0001},
	
	
	{{{MILLISECONDS_TO_SAMPLES(300), MILLISECONDS_TO_SAMPLES(100)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_480, TGN_COEF_480}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	0x0000, 0x0001},
	
	
	{{{MILLISECONDS_TO_SAMPLES(2000), MILLISECONDS_TO_SAMPLES(100)}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
	{{TGN_YN_2_1000, TGN_COEF_1000}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
  {{0},{0},{0},{0}},
	0x0000, 0x0001},
	
	

	
    {{{BEEP_REC_ON, BEEP_REC_OFF}, {0x0000, BEEP_REC_OFF}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_1400, TGN_COEF_1400}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}}, 
    {{0},{0},{0},{0}},
    TGN_INFINITE_REPEAT, 0x0001},

	
    {{{BEEP_REC_ON, BEEP_REC_OFF}, {0x0000, BEEP_REC_OFF}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_1400, TGN_COEF_1400}, {0x0000, 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}}, 
    {{0},{0},{0},{0}},
    TGN_INFINITE_REPEAT, 0x0001},

	
    {{{BEEP_MON_ON1, BEEP_MON_OFF1}, {BEEP_MON_ON2, BEEP_MON_OFF2},{0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_480, TGN_COEF_480}, {TGN_YN_2_480, TGN_COEF_480}, {0x0000, 0x0000}, {0x0000, 0x0000}}, 
    {{0},{0},{0},{0}},
    TGN_INFINITE_REPEAT, 0x0011},

	
    {{{MILLISECONDS_TO_SAMPLES(333), 0x0000}, {MILLISECONDS_TO_SAMPLES(333), 0x0000}, {0x0000, 0x0000}, {0x0000, 0x0000}},
    {{TGN_YN_2_425, TGN_COEF_425}, {TGN_YN_2_300, TGN_COEF_300}, {0x0000, 0x0000}, {0x0000, 0x0000}}, 
    {{0},{0},{0},{0}},
    0x0002, 0x0011},





















































};


WebrtcToneGenerator::WebrtcToneGenerator( ToneType type )
{
	TONE_TABLE_TYPE *tone = &ToneTable[type];

	
	m_SinewaveIdx = 0;
	m_CadenceIdx = 0;
	memcpy( m_Cadence, tone->Cadence, sizeof( m_Cadence ) );
	for ( int i = 0; i < MAX_TONEGENS; i++ )
	{
		m_Sinewave[i].Coef = tone->Coefmem[i].FilterCoef;
		m_Sinewave[i].Yn_1 = 0;
		m_Sinewave[i].Yn_2 = tone->Coefmem[i].FilterMemory;
	}
	m_RepeatCount = tone->RepeatCount;
	m_Descriptor  = tone->Descriptor;

	
	m_Sample = m_Cadence[0];
}


void WebrtcToneGenerator::ToneGen( PSINEWAVE param, short *dst, unsigned long length, unsigned long numTones )
{
	unsigned long j;
	unsigned long i;
	long  A, B;
	short T;

	memset( dst, 0, length * sizeof( short ) );

	for ( i = 0; i < numTones; i++ )
	{
		for ( j = 0; j < length; j++ )
		{
			A = -(((long)param->Yn_2) << 15);
			T = param->Yn_1;
			param->Yn_2 = param->Yn_1;
			B = T * ((long)param->Coef) * 2;
			A = A + B;

			
			if ( A >= (long)2147483647 )
			{
				A = (long)2147483647;
			}
			if ( A <= (long)-2147483647 )
			{
				A = (long)-2147483647;
			}
			param->Yn_1 = (short)(A >> 15);
			dst[j] += (short)(A >> 15);
		}
		param++;
	}
}


int WebrtcToneGenerator::Read( void *buf, int len )
{
	return TGNGenerateTone( (short *)buf, (unsigned long)(len/sizeof(short)) ) ? len : 0;
}


bool WebrtcToneGenerator::TGNGenerateTone( short *dst, unsigned long length )
{
	unsigned long numTone = 0;

	
	if ( m_Sample == 0 )
	{
		

		
		m_CadenceIdx = (m_CadenceIdx + 1) & (MAX_CADENCES - 1);

		
		while ( (m_CadenceIdx != 0) && (m_Cadence[m_CadenceIdx] == 0) )
		{
			m_CadenceIdx = (m_CadenceIdx + 1) & (MAX_CADENCES - 1);
		}

		
		m_Sample = m_Cadence[m_CadenceIdx];

		
		if ( m_CadenceIdx == 0 )
		{
			
			m_SinewaveIdx = 0;

			
			if ( m_RepeatCount != TGN_INFINITE_REPEAT )
			{
				
				if ( m_RepeatCount <= 0 )
				{
					return false;
				}
				else
				{
					m_RepeatCount--;
				}
			}
		}
	}

	
	if ( (m_CadenceIdx & 0x1) == 0 )
	{
		numTone = (m_Descriptor >> (m_CadenceIdx << 1)) & 0xf;

		ToneGen( &m_Sinewave[m_SinewaveIdx], dst, length, numTone );
	}
	else	
	{
		memset( dst, 0, length * sizeof( short ) );
	}

	if ( m_Sample != TGN_INFINITE_REPEAT )
	{
		if ( (length) < m_Sample )
		{
			m_Sample -= length;
		}
		else
		{
			m_Sample = 0;
		}

		if ( !m_Sample )
		{
			
			if ( (m_CadenceIdx & 0x1) == 0 )
			{
				m_SinewaveIdx += numTone;
			}
		}
	}
	return true;
}

} 

#endif
