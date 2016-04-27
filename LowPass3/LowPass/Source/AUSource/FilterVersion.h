#ifndef _FilterVersion_h
#define _FilterVersion_h

#ifdef DEBUG
	#define kFilterVersion 0xFFFFFFFF
#else
	#define kFilterVersion 0x00010000	
#endif

#define Filter_COMP_SUBTYPE		'lowp'
#define Filter_COMP_MANF		'gabe'

#endif

