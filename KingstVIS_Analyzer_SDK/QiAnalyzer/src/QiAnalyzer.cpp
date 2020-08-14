#include "QiAnalyzer.h"
#include "QiAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <windows.h>

QiAnalyzer::QiAnalyzer()
    : Analyzer(),
      mSettings(new QiAnalyzerSettings()),
      mSimulationInitilized(false)
{
    SetAnalyzerSettings(mSettings.get());
}

QiAnalyzer::~QiAnalyzer()
{
    KillThread();
}


static INT8 cal_diff(U64 cnt1, U64 cnt2)
{
    U64 dif;
    if (cnt1 > cnt2)
        dif = cnt1 -cnt2;
    else
        dif = cnt2 - cnt1;

    if (dif < 27000 && dif > 23000)
        return 1;
    else if (dif < 53000 && dif > 47000)
        return 0;
    else
        return -1;
}

void QiAnalyzer::ComputeSampleOffsets()
{
    ClockGenerator clock_generator;
    U64 tmp;
    clock_generator.Init(mSettings->mBitRate, mSampleRateHz);

    mSampleOffsets.clear();

    U32 num_bits = mSettings->mBitsPerTransfer;

    if (mSettings->mQiMode != QiAnalyzerEnums::Normal) {
        num_bits++;
    }

    mSampleOffsets.push_back(clock_generator.AdvanceByTimeS(250e-6));  //point to the center of the 1st bit (past the start bit)

    for (U32 i = 0; i < num_bits; i++) {
        tmp = clock_generator.AdvanceByHalfPeriod();
        mSampleOffsets.push_back(clock_generator.AdvanceByTimeS(250e-6));
    }

    if (mSettings->mParity != AnalyzerEnums::None) {
        mParityBitOffset = clock_generator.AdvanceByTimeS(250e-6);
    }

    //to check for framing errors, we also want to check
    //1/2 bit after the beginning of the stop bit
    mStartOfStopBitOffset = clock_generator.AdvanceByTimeS(250e-6);   //i.e. moving from the center of the last data bit (where we left off) to 1/2 period into the stop bit

    //and 1/2 bit before end of the stop bit period
    mEndOfStopBitOffset = clock_generator.AdvanceByHalfPeriod(mSettings->mStopBits - 1.0);  //if stopbits == 1.0, this will be 0
}

void QiAnalyzer::SetupResults()
{
    //Unlike the worker thread, this function is called from the GUI thread
    //we need to reset the Results object here because it is exposed for direct access by the GUI, and it can't be deleted from the WorkerThread

    mResults.reset(new QiAnalyzerResults(this, mSettings.get()));
    SetAnalyzerResults(mResults.get());
    mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}


int QiAnalyzer::Qi_bit_cal(AnalyzerChannelData * pQi)
{
    U64 frame_cnt1, frame_cnt2;
    frame_cnt1 = pQi->GetSampleNumber();
    /* 0->1 */
    pQi->AdvanceToNextEdge();
    frame_cnt2 = pQi->GetSampleNumber();
    if (cal_diff(frame_cnt1, frame_cnt2) == 0) {
        mResults->AddMarker(frame_cnt2, AnalyzerResults::Dot, mSettings->mInputChannel);
        return 0;
    } else if (cal_diff(frame_cnt1, frame_cnt2) == 1) {
        frame_cnt1 = pQi->GetSampleNumber();
        /* 0->1 */
        pQi->AdvanceToNextEdge();
        frame_cnt2 = pQi->GetSampleNumber();
        if (cal_diff(frame_cnt1, frame_cnt2) == 1) {
            mResults->AddMarker(frame_cnt2, AnalyzerResults::Dot, mSettings->mInputChannel);
            return 1;
        }
    }

    mResults->AddMarker(frame_cnt2, AnalyzerResults::Dot, mSettings->mInputChannel);
    return -1;
}

void QiAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();
    /* Sets the length of each bit of the protocol */
    ComputeSampleOffsets();
    U32 num_bits = mSettings->mBitsPerTransfer;
    static bool should_detect_preamble = true;

    should_detect_preamble = true;
    if (mSettings->mQiMode != QiAnalyzerEnums::Normal) {
        num_bits++;
    }

    if (mSettings->mInverted == false) {
        mBitHigh = BIT_HIGH;
        mBitLow = BIT_LOW;
    } else {
        mBitHigh = BIT_LOW;
        mBitLow = BIT_HIGH;
    }

    U64 bit_mask = 0;
    U64 mask = 0x1ULL;
    BOOL start;
    for (U32 i = 0; i < num_bits; i++) {
        bit_mask |= mask;
        mask <<= 1;
    }

    mQi = GetAnalyzerChannelData(mSettings->mInputChannel);
    mQi->TrackMinimumPulseWidth();

    /* find start data frame */
retry:
    if (mQi->GetBitState() == mBitHigh) {
        mQi->AdvanceToNextEdge();
    }


    U64 frame_starting_pos = mQi->GetSampleNumber();
    /* 0->1 */
    mQi->AdvanceToNextEdge();
    U64 frame_starting_pos_2 = mQi->GetSampleNumber();
    if (frame_starting_pos_2 - frame_starting_pos > 80000) {
        start = true;
    } else {
        start = false;
        goto retry;
    }

    /* if goto here, start parsing protocol */
    for (; ;) {
        //we're now at the beginning of the start bit.  We can start collecting the data.
        U64 frame_starting_sample = mQi->GetSampleNumber();
        static U8 preamble_cnt;
        U64 data = 0;
        BOOL last_status, current_status;
        bool framing_error = false;
        bool mp_is_address = false;


        DataBuilder data_builder;
        data_builder.Reset(&data, mSettings->mShiftOrder, num_bits);
        U64 marker_location = frame_starting_sample;
        U64 frame_cnt1, frame_cnt2;
        
        preamble_cnt = 0;
        if (should_detect_preamble == true) {
            for (INT i = 0; i < 25; i++) {
                BOOL tmp_preamble_bit;
                tmp_preamble_bit = Qi_bit_cal(mQi);
                if (tmp_preamble_bit == 0) {
                    /* find start bit */
                    break;
                } else if (tmp_preamble_bit == 1) {
                    preamble_cnt++;
                }
            }
        }

        BOOL tmp_start_bit;

        /* parsing 8bit */
        for (U32 i = 0; i < num_bits; i++) {
            tmp_start_bit = Qi_bit_cal(mQi);
            if (tmp_start_bit) {
                data_builder.AddBit(BIT_HIGH);
            } else {
                data_builder.AddBit(BIT_LOW);
            }
        }

        /* get parity bit */
        Qi_bit_cal(mQi);

        /* get stop bit */
        tmp_start_bit = Qi_bit_cal(mQi);

        U64 this_bit_length = mQi->GetSampleNumber();
        U64 next_bit_length = mQi->GetSampleOfNextEdge();

        if (next_bit_length - this_bit_length > 80000) {
            should_detect_preamble = true;
        } else {
            should_detect_preamble = false;
            Qi_bit_cal(mQi);
        }

        //ok now record the value!
        //note that we're not using the mData2 or mType fields for anything, so we won't bother to set them.
        Frame frame;
        frame.mStartingSampleInclusive = frame_starting_sample;
        frame.mEndingSampleInclusive = mQi->GetSampleNumber();
        frame.mData1 = data;
        frame.mFlags = 0;

        mResults->AddFrame(frame);

        mResults->CommitResults();

        ReportProgress(frame.mEndingSampleInclusive);
        CheckIfThreadShouldExit();
    }
}

bool QiAnalyzer::NeedsRerun()
{
    if (mSettings->mUseAutobaud == false) {
        return false;
    }

    //ok, lets see if we should change the bit rate, base on mShortestActivePulse

    U64 shortest_pulse = mQi->GetMinimumPulseWidthSoFar();

    if (shortest_pulse == 0) {
        AnalyzerHelpers::Assert("Alg problem, shortest_pulse was 0");
    }

    U32 computed_bit_rate = U32(double(mSampleRateHz) / double(shortest_pulse));

    if (computed_bit_rate > mSampleRateHz) {
        AnalyzerHelpers::Assert("Alg problem, computed_bit_rate is higer than sample rate");    //just checking the obvious...
    }

    if (computed_bit_rate > (mSampleRateHz / 4)) {
        return false;    //the baud rate is too fast.
    }
    if (computed_bit_rate == 0) {
        //bad result, this is not good data, don't bother to re-run.
        return false;
    }

    U32 specified_bit_rate = mSettings->mBitRate;

    double error = double(AnalyzerHelpers::Diff32(computed_bit_rate, specified_bit_rate)) / double(specified_bit_rate);

    if (error > 0.1) {
        mSettings->mBitRate = computed_bit_rate;
        mSettings->UpdateInterfacesFromSettings();
        return true;
    } else {
        return false;
    }
}

U32 QiAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor **simulation_channels)
{
    if (mSimulationInitilized == false) {
        mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 QiAnalyzer::GetMinimumSampleRateHz()
{
    return mSettings->mBitRate * 4;
}

const char *QiAnalyzer::GetAnalyzerName() const
{
    return "Qi";
}

const char *GetAnalyzerName()
{
    return "Qi";
}

Analyzer *CreateAnalyzer()
{
    return new QiAnalyzer();
}

void DestroyAnalyzer(Analyzer *analyzer)
{
    delete analyzer;
}

