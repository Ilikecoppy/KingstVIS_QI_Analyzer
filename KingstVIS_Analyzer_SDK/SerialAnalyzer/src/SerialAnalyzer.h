#ifndef SERIAL_ANALYZER_H
#define SERIAL_ANALYZER_H

#include <Analyzer.h>
#include "SerialAnalyzerResults.h"
#include "SerialSimulationDataGenerator.h"

class SerialAnalyzerSettings;

class ANALYZER_EXPORT SerialAnalyzer : public Analyzer
{
public:
    SerialAnalyzer();
    virtual ~SerialAnalyzer();
    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);
    virtual U32 GetMinimumSampleRateHz();

    virtual const char *GetAnalyzerName() const;
    virtual bool NeedsRerun();


#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class

protected: //functions
    void ComputeSampleOffsets();

protected: //vars
    std::auto_ptr< SerialAnalyzerSettings > mSettings;
    std::auto_ptr< SerialAnalyzerResults > mResults;
    AnalyzerChannelData *mSerial;

    SerialSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

    //Serial analysis vars:
    U32 mSampleRateHz;
    std::vector<U32> mSampleOffsets;
    U32 mParityBitOffset;
    U32 mStartOfStopBitOffset;
    U32 mEndOfStopBitOffset;
    BitState mBitLow;
    BitState mBitHigh;

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);

#endif //SERIAL_ANALYZER_H
