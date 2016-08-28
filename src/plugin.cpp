#include "plugin.h"

#include <QDir>

QString Plugin::plugDir = "..";
QString Plugin::plugNamePattern = "libPiCollection%1";
//TODO: Fix .so extension in case of windows
QString Plugin::plugExt = ".so";

Plugin::Plugin(const QString &type, IMainLog *log)
{
    m_PlugLib.setFileName(plugDir + "/" + plugNamePattern.arg(type) + plugExt);
    if (!m_PlugLib.load()) {
        log->LogOut("Failed to load lib " + m_PlugLib.fileName() +  ". Error:"
                    + m_PlugLib.errorString());
        throw std::bad_function_call();
    }
    m_fSiteInfo = (ISiteInfoCtr_t*)m_PlugLib.resolve("ISiteInfoCtr");
    m_fFsBldCtr = (IFileSysBldrCtr_t*)m_PlugLib.resolve("IFileSysBldrCtr");
    m_fUrlBldr = (IUrlBuilderCtr_t*)m_PlugLib.resolve("IUrlBuilderCtr");
    m_fHtmlPageElm = ((IHtmlPageElmCtr_t*)m_PlugLib.resolve("IHtmlPageElmCtr"));
    if (!m_fHtmlPageElm) {
        throw std::bad_function_call();
    }
}

std::shared_ptr<ISiteInfo> Plugin::GetSiteInfo()
{
    return m_fSiteInfo();
}

std::shared_ptr<IUrlBuilder> Plugin::GetUrlBuilder()
{
    return m_fUrlBldr();
}

std::shared_ptr<IFileSysBldr> Plugin::GetFileNameBuilder()
{
    return m_fFsBldCtr();
}

std::shared_ptr<IHtmlPageElm> Plugin::GetHtmlPageElm(const QString &strContent)
{
    assert(m_fHtmlPageElm);
    return m_fHtmlPageElm(strContent.toStdWString());
}

QStringList Plugin::GetPluginTypes()
{
    QStringList retVal;
    QDir dir(plugDir);
    auto lstFiles = dir.entryList(
                (plugNamePattern + plugExt).arg("*").split(" "), QDir::Files);
    foreach(auto file, lstFiles) {
        file.replace(plugNamePattern.arg(""), "").replace(plugExt, "");
        retVal.push_back(file);
    }
    return retVal;
}
