#ifndef Qi_ANALYZER_SETTINGS
#define Qi_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

namespace QiAnalyzerEnums
{
    enum Mode { Normal, MpModeMsbZeroMeansAddress, MpModeMsbOneMeansAddress };
};

class QiAnalyzerSettings : public AnalyzerSettings

{
public:
    QiAnalyzerSettings();
    virtual ~QiAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings(const char *settings);
    virtual const char *SaveSettings();

    Channel mInputChannel;
    U32 mBitRate;
    U32 mBitsPerTransfer;
    AnalyzerEnums::ShiftOrder mShiftOrder;
    double mStopBits;
    AnalyzerEnums::Parity mParity;
    bool mInverted;
    bool mUseAutobaud;
    QiAnalyzerEnums::Mode mQiMode;

protected:
    std::auto_ptr< AnalyzerSettingInterfaceChannel >    mInputChannelInterface;
};

#endif //Qi_ANALYZER_SETTINGS
