// LowPass.h
// Header for the low pass AUEffectBase child class and AUKernalBase child class
#ifndef _LowPass_h
#define _LowPass_h

#define kNumResponseFrequencies 512

enum{ kAudioUnitCustomProperty_FilterFrequencyResponse = 65536 };

typedef struct Response{
	Float64 mFreq;
	Float64 mMag;
} Response;



#pragma mark ____FilterKernel

// DSP module for filtering
class FilterKernel : public AUKernelBase {
public:
	FilterKernel(AUEffectBase *inAudioUnit ) : AUKernelBase(inAudioUnit) { Reset(); }
	virtual ~FilterKernel() {}
    
	// process non-interleaved samples
	virtual void Process(const Float32 *inputSource, Float32 *inputDest, UInt32 inputFrames, UInt32 inputNumChannels, bool& ioSilence);
    
    
	// resets the filter state
	virtual void Reset();
    
	// calculate the lowpass coefficients
	void CalculateFilterCoefficients(double, double);
    
private:
	// coefficients
	double A[3];
	double B[3];
	// state
	double X[2];
	double Y[2];
	// remember last parameters so we know when to recalculate them
	double lastCutoff;
	double lastRes;
};


#pragma mark ____Filter
// Audio Unit effect class
class Filter : public AUEffectBase {
public:
	Filter(AudioUnit component);
    
	virtual OSStatus Version() { return kFilterVersion; }
	virtual OSStatus Initialize() { return AUEffectBase::Initialize(); }
    
	virtual AUKernelBase* NewKernel() { return new FilterKernel(this); }
    
	// Custom property overrides
	virtual OSStatus GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID, AudioUnitParameterInfo& outParameterInfo);
	virtual OSStatus GetPropertyInfo(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, UInt32& outDataSize, Boolean& outWritable);
	virtual OSStatus GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void* outData);
	
	// Preset handling overrides
	virtual OSStatus GetPresets(CFArrayRef *outData) const;
	virtual OSStatus NewFactoryPresetSet(const AUPreset& inNewFactoryPreset);
    
	// Report 1ms tail
	virtual	bool SupportsTail() { return true; }
	virtual Float64 GetTailTime() {return 0.001; }
    
	// "no latency"
	virtual Float64 GetLatency() { return 0.0; }
    
protected:
};



#endif
