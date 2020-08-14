#ifndef SPI_ANALYZER_H
#define SPI_ANALYZER_H

#include <Analyzer.h>
#include "SpiAnalyzerResults.h"
#include "SpiSimulationDataGenerator.h"

class SpiAnalyzerSettings;

class ANALYZER_EXPORT SpiAnalyzer : public Analyzer
{
public:
    SpiAnalyzer();
    virtual ~SpiAnalyzer();
    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);
    virtual U32 GetMinimumSampleRateHz();

    virtual const char *GetAnalyzerName() const;
    virtual bool NeedsRerun();

protected: //functions
    void Setup();
    void AdvanceToActiveEnableEdge();
    bool IsInitialClockPolarityCorrect();
    void AdvanceToActiveEnableEdgeWithCorrectClockPolarity();
    bool WouldAdvancingTheClockToggleEnable();
    void GetWord();

#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class
protected:  //vars
    std::auto_ptr< SpiAnalyzerSettings > mSettings;
    std::auto_ptr< SpiAnalyzerResults > mResults;
    bool mSimulationInitilized;
    SpiSimulationDataGenerator mSimulationDataGenerator;

    AnalyzerChannelData *mMosi;
    AnalyzerChannelData *mMiso;
    AnalyzerChannelData *mClock;
    AnalyzerChannelData *mEnable;

    U64 mCurrentSample;
    AnalyzerResults::MarkerType mArrowMarker;
    std::vector<U64> mArrowLocations;

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);

#endif //SPI_ANALYZER_H
