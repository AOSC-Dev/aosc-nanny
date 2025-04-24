#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QLayout>
#include <QMessageBox>
#include <QSpacerItem>
#include <QStandardPaths>
#include <QString>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFormat>

#include <KAboutData>
#include <KLocalizedString>

#include <sys/auxv.h>

static QString formatTelemetryWarning(const QString &name,
                                      const QString &altSoftware,
                                      const QString &altPackage,
                                      const QString &description,
                                      const QString &eulaUrl,
                                      const QString &legalDocName)
{
    QString promptText =
        i18n(
            "%1 may collect your usage data on an opt-out basis, per the <a href=\"%2\">%3</a>.<br><br>This default setting does not comply with our "
            "guidelines on "
            "telemetry in packaged software, per section 5 of the <a "
            "href=\"https://wiki.aosc.io/developer/packaging/package-styling-manual/#package-features\">AOSC OS Packaging Styling Manual</a>. ",
            description,
            eulaUrl,
            legalDocName.isEmpty() ? i18n("Licensing Terms") : legalDocName)
        % (altSoftware.isEmpty() ? QStringLiteral("<br><br>")
                                 : i18n("We offer a Telemetry-free alternative, %1 (package: %2).<br><br>", altSoftware, altPackage))
        % i18n("Would you like to proceed with launching %1?", name);

    return promptText;
}

static QString formatCpuBaselineError(const QString &feature, const QString &name)
{
    return i18n(
        "Your processor does not support the \"%1\" feature, which is required by %2. This application will therefore not function correctly on your device "
        "and will now terminate.",
        feature,
        name);
}

static bool shouldPrompt(const QString &name)
{
    const QString configPath = QStandardPaths::locate(QStandardPaths::ConfigLocation, QLatin1String("nanny.db"));
    if (configPath.isEmpty())
        return true;
    QFile configFile(configPath);
    if (!configFile.open(QFile::ReadOnly))
        return true;
    const QByteArray configText = configFile.readAll();
    const QString searchString = name % QLatin1Char('\n');

    return !configText.contains(searchString.toUtf8());
}

static bool saveRecord(const QString &name)
{
    const QString configPath = QStandardPaths::locate(QStandardPaths::ConfigLocation, QLatin1String("nanny.db"));
    if (configPath.isEmpty())
        return false;
    QFile configFile(configPath);
    if (!configFile.open(QFile::WriteOnly | QFile::Append))
        return false;
    configFile.write(name.toUtf8() + QByteArray("\n"));
    return true;
}

#if defined(__riscv) || defined(_ARCH_PPC64)
#define AOSC_NANNY_USE_AUXV 1
#endif

#ifdef AOSC_NANNY_USE_AUXV
static bool checkCpuFeatureAuxv(const QString &feature)
{
    const unsigned long auxv_p1 = getauxval(AT_HWCAP);
    const unsigned long auxv_p2 = getauxval(AT_HWCAP2);

    // AT_HWCAP masks
    const QHash<const char *, unsigned long> capabilities_p1 = {
        // ppc64le (https://elixir.bootlin.com/linux/v6.14.3/source/arch/powerpc/include/uapi/asm/cputable.h)
        {"altivec", 0x10000000},
        {"vsx", 0x00000080},
        {"spe", 0x00800000},
        // riscv64 (https://elixir.bootlin.com/linux/v6.14.3/source/arch/riscv/include/asm/hwcap.h)
        {"v", 1UL << ('v' - 'a')},
        {"h", 1UL << ('h' - 'a')},
        {"zicsr", 1UL << 40},
        {"zifencei", 1UL << 41},
    };

    // AT_HWCAP2 masks
    const QHash<const char *, unsigned long> capabilities_p2 = {
        // ppc64le
        {"mma", 0x00020000},
        {"vec_crypto", 0x02000000},
        // riscv64
        {"zfh", 1UL << (66 - 64)},
        {"zvfh", 1UL << (69 - 64)},
    };

    const char *featureLower = feature.toLower().toUtf8().data();
    if (unsigned long p1 = capabilities_p1.value(featureLower, 0)) {
        return (auxv_p1 & p1) > 0;
    } else if (unsigned long p2 = capabilities_p2.value(featureLower, 0)) {
        return (auxv_p2 & p2) > 0;
    }

    return false;
}
#endif

static bool checkCpuFeature(const QString &feature)
{
#ifdef AOSC_NANNY_USE_AUXV
    return checkCpuFeatureAuxv(feature);
#else
    QFile file(QStringLiteral("/proc/cpuinfo"));
    file.open(QFile::ReadOnly);
    if (!file.isOpen())
        return false;
    const QByteArray data = file.readAll();
    constexpr const char *searchPrefix =
#if defined(__loongarch__) || defined(__ARM_ARCH)
        "Features";
#elif defined(__mips__)
        "ASEs implemented";
#else
        "flags";
#endif

    const int prefixPos = data.indexOf(searchPrefix);
    if (prefixPos < 0)
        return false;
    const int commaPos = data.indexOf(':', prefixPos);
    if (commaPos < 0)
        return false;
    const int endPos = data.indexOf('\n', commaPos);
    if (commaPos < 0)
        return false;
    const QByteArray flags = data.mid(commaPos + 1, endPos - commaPos - 1).trimmed();

    return flags.contains(feature.toUtf8());
#endif // ! AOSC_NANNY_USE_AUXV
}

static bool showWarningGui(const QString &title, const QString &text, const QString &promptText)
{
    const QString bodyText = text % QStringLiteral("<br>") % promptText;
    const QIcon icon = QIcon::fromTheme(QStringLiteral("dialog-warning"));
    QMessageBox messageBox{QMessageBox::Icon::Warning, title, bodyText};
    messageBox.setWindowFlag(Qt::WindowType::WindowStaysOnTopHint, true);
    messageBox.setWindowIcon(icon);
    messageBox.setModal(true);
    // HACK: coming from https://forum.qt.io/topic/56624/setting-the-size-of-a-qmessagebox/3
    // this is needed to stretch the message box to the desired size
    QSpacerItem *horizontalSpacer = new QSpacerItem{640, 0, QSizePolicy::Minimum, QSizePolicy::Expanding};
    QGridLayout *internalLayout = static_cast<QGridLayout *>(messageBox.layout());
    internalLayout->addItem(horizontalSpacer, internalLayout->rowCount(), 0, 1, internalLayout->columnCount());

    if (promptText.isEmpty()) {
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
        return false;
    }
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    messageBox.setButtonText(QMessageBox::Yes, i18n("Continue"));
    messageBox.setButtonText(QMessageBox::No, i18n("Quit"));
    messageBox.setDefaultButton(QMessageBox::No);
    return messageBox.exec() == QMessageBox::Yes;
}

static bool showWarningCli(const QString &title, const QString &text, const QString &promptText)
{
    const QString promptToShow =
        promptText.isEmpty() ? QString{} : QLatin1Char('\n') % promptText % QLatin1String("\n\n") % i18n("Proceed? [y/N]") % QLatin1Char(' ');
    const QString bodyText = title % QLatin1String("\n\n") % text % promptToShow;
    QFile stdout{};
    QFile stdin{};
    if (!stdout.open(1, QIODevice::WriteOnly)) {
        QApplication::exit(209); // EXIT_STDOUT
        // Qt's exit is not immediate, so we still need to return here
        return false;
    }
    if (!stdin.open(0, QIODevice::ReadOnly)) {
        QApplication::exit(210); // EXIT_STDIN
        // Qt's exit is not immediate, so we still need to return here
        return false;
    }
    stdout.write(bodyText.toUtf8());
    stdout.flush();
    if (promptText.isEmpty())
        return false;
    const QByteArray response = stdin.readLine(64);
    if (response.length() >= 1 && (response[0] == 'y' || response[0] == 'Y')) {
        return true;
    }
    const QString rejectedText = i18n("You have chosen not to proceed. Exiting...") + QLatin1Char('\n');
    stdout.write(rejectedText.toUtf8());
    stdout.flush();
    return false;
}

static QString translateHTMLforCli(const QString &text)
{
    if (text.isEmpty())
        return text;

    QTextDocument qdoc{};
    qdoc.setHtml(text);
    const bool renderColors = getenv("NO_COLOR") == nullptr; // respect NO_COLOR environment variable
    if (!renderColors)
        return qdoc.toPlainText();

    QString richCliOutput{};
    for (QTextBlock block = qdoc.begin(); block != qdoc.end(); block = block.next()) {
        for (auto it = block.begin(); it != block.end(); it++) {
            const QTextFragment fragment = it.fragment();
            const QTextCharFormat format = fragment.charFormat();
            const QString &link = format.anchorHref();
            if (!link.isEmpty()) {
                richCliOutput += QStringLiteral("\x1b]8;;%1\x1b\\%2\x1b]8;;\x1b\\(%1)").arg(link).arg(fragment.text());
                continue;
            }
            const QString normalized = fragment.text().replace(QChar(0x2029), QLatin1Char('\n')).replace(QChar(0x2028), QLatin1Char('\n'));
            richCliOutput += normalized;
        }
    }
    if (!richCliOutput.endsWith(QLatin1Char('\n'))) {
        richCliOutput += QLatin1Char('\n');
    }
    return richCliOutput;
}

static bool showPrompt(const QString &title, const QString &text, const QString &promptText, const bool forceText)
{
    const bool useGui = (!forceText && getenv("DISPLAY") != nullptr); // If $DISPLAY is set, we're running in a graphical environment
    return useGui ? showWarningGui(title, text, promptText) : showWarningCli(title, translateHTMLforCli(text), translateHTMLforCli(promptText));
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("aosc-nanny");

    KAboutData aboutData(QStringLiteral("aosc-nanny"),
                         i18nc("@title", "AOSC Nanny"),
                         QStringLiteral(PROJECT_VERSION),
                         i18nc("@info", "Application advisory system for AOSC OS."),
                         KAboutLicense::LicenseKey::GPL_V2);

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser{};
    QCommandLineOption nameOption(QStringLiteral("n"), i18nc("@info:shell", "Name of the offending package."), i18nc("@info:shell value name", "Package Name"));
    parser.addOption(nameOption);

    QCommandLineOption altSoftwareOption(QStringLiteral("a"),
                                         i18nc("@info:shell", "Name of alternative software (if applicable)."),
                                         i18nc("@info:shell value name", "Alternative Software"));
    parser.addOption(altSoftwareOption);

    QCommandLineOption altPackageOption(QStringLiteral("k"),
                                        i18nc("@info:shell", "Name of alternative package (if applicable).\nYou pass -a with -k."),
                                        i18nc("@info:shell value name", "Alternative Package"));
    parser.addOption(altPackageOption);

    QCommandLineOption descriptionOption(QStringLiteral("d"),
                                         i18nc("@info:shell", "Description of the offending package (usually the \"pretty name\" for said application)."),
                                         i18nc("@info:shell value name", "Pretty Name"));
    parser.addOption(descriptionOption);

    QCommandLineOption EULAOption(QStringLiteral("l"), i18nc("@info:shell", "URL to the licensing terms."), i18nc("@info:shell value name", "EULA_URL"));
    parser.addOption(EULAOption);

    QCommandLineOption CpuFeatureOption(QStringLiteral("f"),
                                        i18nc("@info:shell", "Required processor feature."),
                                        i18nc("@info:shell value name", "CPU Feature"));
    parser.addOption(CpuFeatureOption);

    QCommandLineOption textModeFlag(QStringLiteral("c"), i18nc("@info:shell", "Launch in command line."));
    parser.addOption(textModeFlag);

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    // check commandline options
    if (!parser.isSet(nameOption)) {
        parser.showHelp(1);
        return 1;
    }
    if (!parser.isSet(descriptionOption) && !parser.isSet(CpuFeatureOption)) {
        parser.showHelp(1);
        return 1;
    }

    // process requests
    const QString name = parser.value(nameOption);
    if (parser.isSet(CpuFeatureOption)) {
        const QString feature = parser.value(CpuFeatureOption);
        // CPU feature is always checked
        if (!checkCpuFeature(feature)) {
            showPrompt(i18n("Warning"), formatCpuBaselineError(feature, name), QString{}, parser.isSet(textModeFlag));
            return 10;
        }
    }
    if (parser.isSet(descriptionOption)) {
        if (!shouldPrompt(name)) {
            return 0;
        }
        if (!parser.isSet(descriptionOption) || !parser.isSet(EULAOption)) {
            parser.showHelp(1);
            return 1;
        }
        const char *legalDocName = getenv("LEGAL_DOC_NAME");
        const QString text = formatTelemetryWarning(parser.value(nameOption),
                                                    parser.value(altSoftwareOption),
                                                    parser.value(altPackageOption),
                                                    parser.value(descriptionOption),
                                                    parser.value(EULAOption),
                                                    QString::fromUtf8(legalDocName));
        const bool accepted = showPrompt(
            i18n("Warning"),
            text,
            i18n("By selecting \"Yes,\" you agree to the licensing terms referenced above and consent launching an application which violates our packaging "
                 "guidelines."),
            parser.isSet(textModeFlag));
        if (accepted) {
            saveRecord(name);
            return 0;
        }
        return 10;
    }

    return 0;
}
