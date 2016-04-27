#ifndef _LowPass_h
#define _LowPass_h

#define kNumResponseFrequencies 512

enum{ kAudioUnitCustomProperty_FilterFrequencyResponse = 65536 };

typedef struct Response{
	Float64 mFreq;
	Float64 mMag;
} Response;

#endif
