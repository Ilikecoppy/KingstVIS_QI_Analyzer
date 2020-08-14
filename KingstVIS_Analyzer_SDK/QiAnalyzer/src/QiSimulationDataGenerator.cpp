#include "QiSimulationDataGenerator.h"
#include "QiAnalyzerSettings.h"

QiSimulationDataGenerator::QiSimulationDataGenerator()
{
}

QiSimulationDataGenerator::~QiSimulationDataGenerator()
{
}

void QiSimulationDataGenerator::Initialize(U32 simulation_sample_rate, QiAnalyzerSettings *settings)
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;

    mClockGenerator.Init(mSettings->mBitRate, simulation_sample_rate);
    mQiSimulationData.SetChannel(mSettings->mInputChannel);
    mQiSimulationData.SetSampleRate(simulation_sample_rate);

    if (mSettings->mInverted == false) {
        mBitLow = BIT_LOW;
        mBitHigh = BIT_HIGH;
    } else {
        mBitLow = BIT_HIGH;
        mBitHigh = BIT_LOW;
    }

    mQiSimulationData.SetInitialBitState(mBitHigh);
    mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod(10.0));  //insert 10 bit-periods of idle

    mValue = 0;

    mMpModeAddressMask = 0;
    mMpModeDataMask = 0;
    mNumBitsMask = 0;

    U32 num_bits = mSettings->mBitsPerTransfer;
    for (U32 i = 0; i < num_bits; i++) {
        mNumBitsMask <<= 1;
        mNumBitsMask |= 0x1;
    }

    if (mSettings->mQiMode == QiAnalyzerEnums::MpModeMsbOneMeansAddress) {
        mMpModeAddressMask = 0x1ull << (mSettings->mBitsPerTransfer);
    }

    if (mSettings->mQiMode == QiAnalyzerEnums::MpModeMsbZeroMeansAddress) {
        mMpModeDataMask = 0x1ull << (mSettings->mBitsPerTransfer);
    }
}

U32 QiSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels)
{
    U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

    while (mQiSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested) {
        if (mSettings->mQiMode == QiAnalyzerEnums::Normal) {
            CreateQiByte(mValue++);

            mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod(10.0));     //insert 10 bit-periods of idle
        } else {
            U64 address = 0x1 | mMpModeAddressMask;
            CreateQiByte(address);

            for (U32 i = 0; i < 4; i++) {
                mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod(2.0));  //insert 2 bit-periods of idle
                CreateQiByte((mValue++ & mNumBitsMask) | mMpModeDataMask);
            };

            mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod(20.0));     //insert 20 bit-periods of idle

            address = 0x2 | mMpModeAddressMask;
            CreateQiByte(address);

            for (U32 i = 0; i < 4; i++) {
                mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod(2.0));  //insert 2 bit-periods of idle
                CreateQiByte((mValue++ & mNumBitsMask) | mMpModeDataMask);
            };

            mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod(20.0));     //insert 20 bit-periods of idle

        }
    }

    *simulation_channels = &mQiSimulationData;


    return 1;  // we are retuning the size of the SimulationChannelDescriptor array.  In our case, the "array" is length 1.
}

void QiSimulationDataGenerator::CreateQiByte(U64 value)
{
    //assume we start high

    mQiSimulationData.Transition();  //low-going edge for start bit
    mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod());    //add start bit time

    if (mSettings->mInverted == true) {
        value = ~value;
    }

    U32 num_bits = mSettings->mBitsPerTransfer;
    if (mSettings->mQiMode != QiAnalyzerEnums::Normal) {
        num_bits++;
    }

    BitExtractor bit_extractor(value, mSettings->mShiftOrder, num_bits);

    for (U32 i = 0; i < num_bits; i++) {
        mQiSimulationData.TransitionIfNeeded(bit_extractor.GetNextBit());
        mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod());
    }

    if (mSettings->mParity == AnalyzerEnums::Even) {

        if (AnalyzerHelpers::IsEven(AnalyzerHelpers::GetOnesCount(value)) == true) {
            mQiSimulationData.TransitionIfNeeded(mBitLow);    //we want to add a zero bit
        } else {
            mQiSimulationData.TransitionIfNeeded(mBitHigh);    //we want to add a one bit
        }

        mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod());

    } else if (mSettings->mParity == AnalyzerEnums::Odd) {

        if (AnalyzerHelpers::IsOdd(AnalyzerHelpers::GetOnesCount(value)) == true) {
            mQiSimulationData.TransitionIfNeeded(mBitLow);    //we want to add a zero bit
        } else {
            mQiSimulationData.TransitionIfNeeded(mBitHigh);
        }

        mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod());

    }

    mQiSimulationData.TransitionIfNeeded(mBitHigh);   //we need to end high

    //lets pad the end a bit for the stop bit:
    mQiSimulationData.Advance(mClockGenerator.AdvanceByHalfPeriod(mSettings->mStopBits));
}
