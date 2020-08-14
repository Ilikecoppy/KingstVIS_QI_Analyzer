﻿#include "SerialAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>

#pragma warning(disable: 4800) //warning C4800: 'U32' : forcing value to bool 'true' or 'false' (performance warning)
#define CHANNEL_NAME "Data"

SerialAnalyzerSettings::SerialAnalyzerSettings()
    :   mInputChannel(UNDEFINED_CHANNEL),
        mBitRate(9600),
        mBitsPerTransfer(8),
        mShiftOrder(AnalyzerEnums::LsbFirst),
        mStopBits(1.0),
        mParity(AnalyzerEnums::None),
        mInverted(false),
        mUseAutobaud(false),
        mSerialMode(SerialAnalyzerEnums::Normal)
{
    mInputChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
    mInputChannelInterface->SetTitleAndTooltip(CHANNEL_NAME, "Standard Async Serial");
    mInputChannelInterface->SetChannel(mInputChannel);

    mBitRateInterface.reset(new AnalyzerSettingInterfaceInteger());
    mBitRateInterface->SetTitleAndTooltip("Bit Rate (Bits/s)",  "Specify the bit rate in bits per second.");
    mBitRateInterface->SetMax(100000000);
    mBitRateInterface->SetMin(1);
    mBitRateInterface->SetInteger(mBitRate);

    mUseAutobaudInterface.reset(new AnalyzerSettingInterfaceBool());
    mUseAutobaudInterface->SetTitleAndTooltip("", "Automatically find the minimum pulse width and calculate the baud rate according to this pulse width.");
    mUseAutobaudInterface->SetCheckBoxText("Use Autobaud");
    mUseAutobaudInterface->SetValue(mUseAutobaud);

    mInvertedInterface.reset(new AnalyzerSettingInterfaceBool());
    mInvertedInterface->SetTitleAndTooltip("", "Specify if the serial signal is inverted");
    mInvertedInterface->SetCheckBoxText("Inverted (RS232)");
    mInvertedInterface->SetValue(mInverted);

    mBitsPerTransferInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mBitsPerTransferInterface->SetTitleAndTooltip("", "Select the number of bits per frame");
    for (U32 i = 1; i <= 64; i++) {
        std::stringstream ss;

        if (i == 1) {
            ss << "1 Bit per Transfer";
        } else if (i == 8) {
            ss << "8 Bits per Transfer (Standard)";
        } else {
            ss << i << " Bits per Transfer";
        }

        mBitsPerTransferInterface->AddNumber(i, ss.str().c_str(), "");
    }
    mBitsPerTransferInterface->SetNumber(mBitsPerTransfer);


    mStopBitsInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mStopBitsInterface->SetTitleAndTooltip("", "Specify the number of stop bits.");
    mStopBitsInterface->AddNumber(1.0, "1 Stop Bit (Standard)", "");
    mStopBitsInterface->AddNumber(1.5, "1.5 Stop Bits", "");
    mStopBitsInterface->AddNumber(2.0, "2 Stop Bits", "");
    mStopBitsInterface->SetNumber(mStopBits);


    mParityInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mParityInterface->SetTitleAndTooltip("", "Specify None, Even, or Odd Parity.");
    mParityInterface->AddNumber(AnalyzerEnums::None, "No Parity Bit (Standard)", "");
    mParityInterface->AddNumber(AnalyzerEnums::Even, "Even Parity Bit", "");
    mParityInterface->AddNumber(AnalyzerEnums::Odd, "Odd Parity Bit", "");
    mParityInterface->SetNumber(mParity);


    mShiftOrderInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mShiftOrderInterface->SetTitleAndTooltip("", "Select if the most significant bit or least significant bit is transmitted first");
    mShiftOrderInterface->AddNumber(AnalyzerEnums::LsbFirst, "Least Significant Bit Sent First (Standard)", "");
    mShiftOrderInterface->AddNumber(AnalyzerEnums::MsbFirst, "Most Significant Bit Sent First", "");
    mShiftOrderInterface->SetNumber(mShiftOrder);

    enum Mode { Normal, MpModeRightZeroMeansAddress, MpModeRightOneMeansAddress, MpModeLeftZeroMeansAddress, MpModeLeftOneMeansAddress };

    mSerialModeInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mSerialModeInterface->SetTitleAndTooltip("Special Mode", "Specify if this is normal, or MP serial (aka multi-drop, MP, multi-processor, 9-bit serial)");
    mSerialModeInterface->AddNumber(SerialAnalyzerEnums::Normal, "None", "");
    mSerialModeInterface->AddNumber(SerialAnalyzerEnums::MpModeMsbZeroMeansAddress, "MP Mode: Address indicated by MSB=0", "(aka MP, multi-processor, 9-bit serial)");
    mSerialModeInterface->AddNumber(SerialAnalyzerEnums::MpModeMsbOneMeansAddress, "MDB Mode: Address indicated by MSB=1", "(aka multi-drop, 9-bit serial)");
    mSerialModeInterface->SetNumber(mSerialMode);

    AddInterface(mInputChannelInterface.get());
    AddInterface(mBitRateInterface.get());
    AddInterface(mUseAutobaudInterface.get());
    AddInterface(mInvertedInterface.get());
    AddInterface(mBitsPerTransferInterface.get());
    AddInterface(mStopBitsInterface.get());
    AddInterface(mParityInterface.get());
    AddInterface(mShiftOrderInterface.get());
    AddInterface(mSerialModeInterface.get());

    AddExportOption(0, "Export as text/csv file");
    AddExportExtension(0, "Text file", "txt");
    AddExportExtension(0, "CSV file", "csv");

    ClearChannels();
    AddChannel(mInputChannel, CHANNEL_NAME, false);
}

SerialAnalyzerSettings::~SerialAnalyzerSettings()
{
}

bool SerialAnalyzerSettings::SetSettingsFromInterfaces()
{
    if (AnalyzerEnums::Parity(U32(mParityInterface->GetNumber())) != AnalyzerEnums::None)
        if (SerialAnalyzerEnums::Mode(U32(mSerialModeInterface->GetNumber())) != SerialAnalyzerEnums::Normal) {
            SetErrorText("Sorry, but we don't support using parity at the same time as MP mode.");
            return false;
        }
    mInputChannel = mInputChannelInterface->GetChannel();
    mBitRate = mBitRateInterface->GetInteger();
    mBitsPerTransfer = U32(mBitsPerTransferInterface->GetNumber());
    mStopBits = mStopBitsInterface->GetNumber();
    mParity = AnalyzerEnums::Parity(U32(mParityInterface->GetNumber()));
    mShiftOrder =  AnalyzerEnums::ShiftOrder(U32(mShiftOrderInterface->GetNumber()));
    mInverted = mInvertedInterface->GetValue();
    mUseAutobaud = mUseAutobaudInterface->GetValue();
    mSerialMode = SerialAnalyzerEnums::Mode(U32(mSerialModeInterface->GetNumber()));

    ClearChannels();
    AddChannel(mInputChannel, CHANNEL_NAME, true);

    return true;
}

void SerialAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterface->SetChannel(mInputChannel);
    mBitRateInterface->SetInteger(mBitRate);
    mBitsPerTransferInterface->SetNumber(mBitsPerTransfer);
    mStopBitsInterface->SetNumber(mStopBits);
    mParityInterface->SetNumber(mParity);
    mShiftOrderInterface->SetNumber(mShiftOrder);
    mInvertedInterface->SetValue(mInverted);
    mUseAutobaudInterface->SetValue(mUseAutobaud);
    mSerialModeInterface->SetNumber(mSerialMode);
}

void SerialAnalyzerSettings::LoadSettings(const char *settings)
{
    SimpleArchive text_archive;
    text_archive.SetString(settings);

    const char *name_string;    //the first thing in the archive is the name of the protocol analyzer that the data belongs to.
    text_archive >> &name_string;
    if (strcmp(name_string, "SerialAnalyzer") != 0) {
        AnalyzerHelpers::Assert("SerialAnalyzer: Provided with a settings string that doesn't belong to us;");
    }

    text_archive >> mInputChannel;
    text_archive >> mBitRate;
    text_archive >> mBitsPerTransfer;
    text_archive >> mStopBits;
    text_archive >> *(U32 *)&mParity;
    text_archive >> *(U32 *)&mShiftOrder;
    text_archive >> mInverted;

    //check to make sure loading it actual works befor assigning the result -- do this when adding settings to an anylzer which has been previously released.
    bool use_autobaud;
    if (text_archive >> use_autobaud) {
        mUseAutobaud = use_autobaud;
    }

    SerialAnalyzerEnums::Mode mode;
    if (text_archive >> *(U32 *)&mode) {
        mSerialMode = mode;
    }

    ClearChannels();
    AddChannel(mInputChannel, CHANNEL_NAME, true);

    UpdateInterfacesFromSettings();
}

const char *SerialAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << "SerialAnalyzer";
    text_archive << mInputChannel;
    text_archive << mBitRate;
    text_archive << mBitsPerTransfer;
    text_archive << mStopBits;
    text_archive << mParity;
    text_archive << mShiftOrder;
    text_archive << mInverted;
    text_archive << mUseAutobaud;
    text_archive << mSerialMode;

    return SetReturnString(text_archive.GetString());
}
