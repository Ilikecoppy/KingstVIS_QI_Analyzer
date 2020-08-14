#ifndef Qi_ANALYZER_H
#define Qi_ANALYZER_H

#include <Analyzer.h>
#include "QiAnalyzerResults.h"
#include "QiSimulationDataGenerator.h"

class QiAnalyzerSettings;

class ANALYZER_EXPORT QiAnalyzer : public Analyzer
{
public:
    QiAnalyzer();
    int Qi_bit_cal(AnalyzerChannelData * pQi);
    virtual ~QiAnalyzer();
    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);
    virtual U32 GetMinimumSampleRateHz();

    virtual const char *GetAnalyzerName() const;
    virtual bool NeedsRerun();


#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'QiAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class

protected: //functions
    void ComputeSampleOffsets();

protected: //vars
    std::auto_ptr< QiAnalyzerSettings > mSettings;
    std::auto_ptr< QiAnalyzerResults > mResults;
    AnalyzerChannelData *mQi;

    QiSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

    //Qi analysis vars:
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

#endif //Qi_ANALYZER_H
