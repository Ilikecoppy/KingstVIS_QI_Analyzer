#include "QiAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>

#pragma warning(disable: 4800) //warning C4800: 'U32' : forcing value to bool 'true' or 'false' (performance warning)
#define CHANNEL_NAME "Data"

QiAnalyzerSettings::QiAnalyzerSettings()
    :   mInputChannel(UNDEFINED_CHANNEL),
        mBitRate(2000),
        mBitsPerTransfer(8),
        mShiftOrder(AnalyzerEnums::LsbFirst),
        mStopBits(1.0),
        mParity(AnalyzerEnums::None),
        mInverted(false),
        mUseAutobaud(false),
        mQiMode(QiAnalyzerEnums::Normal)
{
    mInputChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
    mInputChannelInterface->SetTitleAndTooltip(CHANNEL_NAME, " Qi");
    mInputChannelInterface->SetChannel(mInputChannel);



    AddInterface(mInputChannelInterface.get());

    AddExportOption(0, "Export as text/csv file");
    AddExportExtension(0, "Text file", "txt");
    AddExportExtension(0, "CSV file", "csv");

    ClearChannels();
    AddChannel(mInputChannel, CHANNEL_NAME, false);
}

QiAnalyzerSettings::~QiAnalyzerSettings()
{
}

bool QiAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannel = mInputChannelInterface->GetChannel();


    ClearChannels();
    AddChannel(mInputChannel, CHANNEL_NAME, true);

    return true;
}

void QiAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterface->SetChannel(mInputChannel);
}

void QiAnalyzerSettings::LoadSettings(const char *settings)
{
    SimpleArchive text_archive;
    text_archive.SetString(settings);

    const char *name_string;    //the first thing in the archive is the name of the protocol analyzer that the data belongs to.
    text_archive >> &name_string;
    if (strcmp(name_string, "QiAnalyzer") != 0) {
        AnalyzerHelpers::Assert("QiAnalyzer: Provided with a settings string that doesn't belong to us;");
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

    QiAnalyzerEnums::Mode mode;
    if (text_archive >> *(U32 *)&mode) {
        mQiMode = mode;
    }

    ClearChannels();
    AddChannel(mInputChannel, CHANNEL_NAME, true);

    UpdateInterfacesFromSettings();
}

const char *QiAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << "QiAnalyzer";
    text_archive << mInputChannel;
    text_archive << mBitRate;
    text_archive << mBitsPerTransfer;
    text_archive << mStopBits;
    text_archive << mParity;
    text_archive << mShiftOrder;
    text_archive << mInverted;
    text_archive << mUseAutobaud;
    text_archive << mQiMode;

    return SetReturnString(text_archive.GetString());
}
