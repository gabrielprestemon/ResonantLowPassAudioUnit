// LowPass.cpp
// source code for Filter class and FilterKernel class
#include "AUEffectBase.h"
#include <AudioToolbox/AudioUnitUtilities.h>
#include "FilterVersion.h"
#include "LowPass.h"
#include <cmath>

AUDIOCOMPONENT_ENTRY(AUBaseProcessFactory, Filter)

// Plug-in parameter enumerations
enum {
	kFilterParam_Cutoff = 0,
	kFilterParam_Res = 1
};

// Plug-in parameter display names
static CFStringRef kCutoff_Name = CFSTR("Cutoff");
static CFStringRef kRes_Name = CFSTR("Resonance");

// Parameter bounds and defaults
const float kMinCutoff = 20.0;
const float kMaxCutoff = 20000.0;
const float kDefaultCutoff = 1000.0;
const float kMinRes = -20.0;
const float kMaxRes = 20.0;
const float kDefaultRes = 0;

// Factory presets
static const int kPreset_One = 0;
static const int kPreset_Two = 1;
static const int kNumPresets = 2;

// Audio Unit preset handling
static AUPreset kPresets[kNumPresets] = {
	{ kPreset_One, CFSTR("Preset One") },
	{ kPreset_Two, CFSTR("Preset Two") }
};


#pragma mark ____Construction_Initialization

Filter::Filter(AudioUnit component) : AUEffectBase(component) {
	SetParameter(kFilterParam_Cutoff, kDefaultCutoff);
	SetParameter(kFilterParam_Res, kDefaultRes);
	
	// Declare sample rate dependency (cutoff)
	SetParamHasSampleRateDependency(true);
}


#pragma mark ____Parameters

// Get parameter info override
OSStatus Filter::GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID, AudioUnitParameterInfo& outParameterInfo) {
	OSStatus result = noErr;

	outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable + kAudioUnitParameterFlag_IsReadable;
		
	if (inScope == kAudioUnitScope_Global) {
		switch(inParameterID)
		{
			case kFilterParam_Cutoff:
				AUBase::FillInParameterName (outParameterInfo, kCutoff_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Hertz;
				outParameterInfo.minValue = kMinCutoff;
				outParameterInfo.maxValue = kMaxCutoff;
				outParameterInfo.defaultValue = kDefaultCutoff;
				outParameterInfo.flags += kAudioUnitParameterFlag_IsHighResolution;
				outParameterInfo.flags += kAudioUnitParameterFlag_DisplayLogarithmic;
				break;
				
			case kFilterParam_Res:
				AUBase::FillInParameterName (outParameterInfo, kRes_Name, false);
				outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
				outParameterInfo.minValue = kMinRes;
				outParameterInfo.maxValue = kMaxRes;
				outParameterInfo.defaultValue = kDefaultRes;
				outParameterInfo.flags += kAudioUnitParameterFlag_IsHighResolution;
				break;
				
			default:
				result = kAudioUnitErr_InvalidParameter;
				break;
		}
	} else {
		result = kAudioUnitErr_InvalidParameter;
	}
	
	return result;
}

#pragma mark ____Properties

// Get property info override
OSStatus Filter::GetPropertyInfo(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, UInt32& outDataSize, Boolean& outWritable) {
	if (inScope == kAudioUnitScope_Global)
	{
		switch (inID)
		{
			case kAudioUnitCustomProperty_FilterFrequencyResponse:	// our custom property
				if(inScope != kAudioUnitScope_Global ) return kAudioUnitErr_InvalidScope;
				outDataSize = kNumberOfResponseFrequencies * sizeof(Response);
				outWritable = false;
				return noErr;
		}
	}
	return AUEffectBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
}

// Get property override
OSStatus Filter::GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void* outData) {
	return AUEffectBase::GetProperty(inID, inScope, inElement, outData);
}


#pragma mark ____Presets

// Get the presets
OSStatus Filter::GetPresets(CFArrayRef* outData) const {
	if (outData == NULL) return noErr;
	
	CFMutableArrayRef theArray = CFArrayCreateMutable (NULL, kNumPresets, NULL);
	for (int i = 0; i < kNumPresets; ++i) {
		CFArrayAppendValue (theArray, &kPresets[i]);
	}

	*outData = (CFArrayRef)theArray;
	return noErr;
}

// Set the factory preset
OSStatus Filter::NewFactoryPresetSet(const AUPreset& inNewFactoryPreset) {
	SInt32 chosenPreset = inNewFactoryPreset.presetNumber;
	
	for(int i = 0; i < kNumPresets; ++i)
	{
		if(chosenPreset == kPresets[i].presetNumber)
		{
			switch(chosenPreset)
			{
				case kPreset_One:
					SetParameter(kFilterParam_Cutoff, 200.0 );
					SetParameter(kFilterParam_Res, -5.0 );
					break;
				case kPreset_Two:
					SetParameter(kFilterParam_Cutoff, 1000.0 );
					SetParameter(kFilterParam_Res, 10.0 );
					break;
			}

			SetAFactoryPresetAsCurrent(kPresets[i]);
			return noErr;
		}
	}
	return kAudioUnitErr_InvalidPropertyValue;
}


#pragma mark ____FilterKernel

// Reset the state of the filter and force coefficient recalculation
void FilterKernel::Reset() {
	X[0] = 0.0;
	X[1] = 0.0;
	Y[0] = 0.0;
	Y[1] = 0.0;
	
	// forces filter coefficient calculation
	lastCutoff = -1.0;
	lastRes = -1.0;
}

// Calculate filter coefficients based on inputs
//    inFreq is normalized frequency (0-1)
//    inRes is db resonance
void FilterKernel::CalculateFilterCoefficients(double inFreq, double inRes) {
	// convert from decibels to linear
	double r = pow(10.0, 0.05 * -inRes);

	double k = 0.5 * r * sin(M_PI * inFreq);
	double c1 = 0.5 * (1.0 - k) / (1.0 + k);
	double c2 = (0.5 + c1) * cos(M_PI * inFreq);
	double c3 = (0.5 + c1 - c2) * 0.25;

	A[0] = 2.0 * c3;
	A[1] = 4.0 * c3;
	A[2] = 2.0 * c3;
	B[1] = 2.0 * -c2;
	B[2] = 2.0 * c1;
}

// process
void FilterKernel::Process(const Float32 *inSource, Float32 *inDest, UInt32 frames, UInt32 inputNumChannels, bool& ioSilence) {
	double cutoff = GetParameter(kFilterParam_Cutoff);
	double res = GetParameter(kFilterParam_Res);
	double nyq = 2.0 / GetSampleRate();

	// cutoff bound check and scale
	cutoff = (cutoff < kMinCutoff) ? kMinCutoff * nyq : cutoff * nyq;
	if(cutoff > 0.99) { cutoff = 0.99; }
	
	// resonance bound check
	if(res < kMinRes) { res = kMinRes; }
	if(res > kMaxRes) { res = kMaxRes; }
	
	// parameter update check
	if(cutoff != lastCutoff || res != lastRes){
		CalculateFilterCoefficients(cutoff, res);
		lastCutoff = cutoff;
		lastRes = res;
	}
	
	// create pointer iterators for the i/o buffers
	const Float32 *source = inSource;
	Float32 *dest = inDest;
	
	// process
	while(frames--){
		float input = *source++;
		
		float output = A[0] * input + A[1] * X[0] + A[2] * X[1] - B[1] * Y[0] - B[2] * Y[1];

		X[1] = X[0];
		X[0] = input;
		Y[1] = Y[0];
		Y[0] = output;
		
		*dest++ = output;
	}
}