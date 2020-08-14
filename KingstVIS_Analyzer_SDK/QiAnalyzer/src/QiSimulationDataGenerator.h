#ifndef Qi_SIMULATION_DATA_GENERATOR
#define Qi_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>

class QiAnalyzerSettings;

class QiSimulationDataGenerator
{
public:
    QiSimulationDataGenerator();
    ~QiSimulationDataGenerator();

    void Initialize(U32 simulation_sample_rate, QiAnalyzerSettings *settings);
    U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);

protected:
    QiAnalyzerSettings *mSettings;
    U32 mSimulationSampleRateHz;
    BitState mBitLow;
    BitState mBitHigh;
    U64 mValue;

    U64 mMpModeAddressMask;
    U64 mMpModeDataMask;
    U64 mNumBitsMask;

protected: //Qi specific

    void CreateQiByte(U64 value);
    ClockGenerator mClockGenerator;
    SimulationChannelDescriptor mQiSimulationData;  //if we had more than one channel to simulate, they would need to be in an array
};

#endif //UNIO_SIMULATION_DATA_GENERATOR
