#include "plugin.h"

#include <assert.h>
#include <QDir>
#include <QLibrary>

QString Plugin::plugDir = "..";
QString Plugin::plugNamePattern = "libPiCollection%1";
#ifdef _WIN32
QString Plugin::plugExt = "dll";
#else
QString Plugin::plugExt = "so";
#endif

Plugin::Plugin(const QString &type, IMainLog *log)
{
    m_pPlugLib = std::make_shared<QLibrary>();
    m_pPlugLib->setFileName(plugDir + "/" + plugNamePattern.arg(type) + "."
                            + plugExt);
    if (!m_pPlugLib->load()) {
        log->LogOut("Failed to load lib " + m_pPlugLib->fileName() +  ". Error:"
                    + m_pPlugLib->errorString());
        throw std::bad_function_call();
    }
    m_fSiteInfo = reinterpret_cast<ISiteInfoCtr_t*>(m_pPlugLib->resolve
                                                    ("ISiteInfoCtr"));
    m_fFsBldCtr = reinterpret_cast<IFileSysBldrCtr_t*>(m_pPlugLib->resolve
                                                       ("IFileSysBldrCtr"));
    m_fUrlBldr = reinterpret_cast<IUrlBuilderCtr_t*>(m_pPlugLib->resolve
                                                     ("IUrlBuilderCtr"));
    m_fHtmlPageElm = reinterpret_cast<IHtmlPageElmCtr_t*>(m_pPlugLib->resolve
                                                          ("IHtmlPageElmCtr"));
    if (!m_fHtmlPageElm) {
        throw std::bad_function_call();
    }
}

std::shared_ptr<ISiteInfo> Plugin::GetSiteInfo() const
{
    return m_fSiteInfo();
}

std::shared_ptr<IUrlBuilder> Plugin::GetUrlBuilder() const
{
    return m_fUrlBldr();
}

std::shared_ptr<IFileSysBldr> Plugin::GetFileNameBuilder() const
{
    return m_fFsBldCtr();
}

std::shared_ptr<IHtmlPageElm> Plugin::GetHtmlPageElm(
        const QString &strContent) const
{
    assert(m_fHtmlPageElm);
    return m_fHtmlPageElm(strContent.toStdWString());
}

QStringList Plugin::GetPluginTypes()
{
    QStringList retVal;
    QDir dir(plugDir);
    auto lstFiles = dir.entryList((plugNamePattern + "." + plugExt).arg("*").
                                  split(" "), QDir::Files);
    for(auto file: lstFiles) {
        file.replace(plugNamePattern.arg(""), "").replace("." + plugExt, "");
        retVal.push_back(file);
    }
    return retVal;
}
