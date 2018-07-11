#include "LinkBackStage.h"

#include <QProcess>
#include <QTimer>
#include <QDebug>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>

#include "DataDefine.h"
#include "ChainIDE.h"
#include "IDEUtil.h"
#include "popwidget/commondialog.h"

#include "datarequire/DataRequireManager.h"

static const int NODE_RPC_PORT = 50320;//node端口  test    formal = test+10
static const int CLIENT_RPC_PORT = 50321;//client端口  test    formal = test+10

class LinkBackStage::DataPrivate
{
public:
    DataPrivate(int type)
        :nodeProc(new QProcess)
        ,clientProc(new QProcess)
        ,chaintype(type)
    {
        nodePort = NODE_RPC_PORT + 10*(type-1);
        clientPort = CLIENT_RPC_PORT + 10*(type-1);
        dataPath = 1 == type ? "/testlink" : "/formallink";

        dataRequire = new DataRequireManager("127.0.0.1",QString::number(clientPort));
    }
    ~DataPrivate()
    {
        delete nodeProc;
        nodeProc = nullptr;
        delete clientProc;
        clientProc = nullptr;
        delete dataRequire;
        dataRequire = nullptr;
    }
public:
    int chaintype;
    int nodePort;
    int clientPort;
    QString dataPath;
    QProcess* nodeProc;
    QProcess* clientProc;
    QTimer    timerForStartExe;
    DataRequireManager *dataRequire;
};

LinkBackStage::LinkBackStage(int type,QObject *parent)
        : BackStageBase(parent)
        ,_p(new DataPrivate(type))
{

}

LinkBackStage::~LinkBackStage()
{
    delete _p;
}

void LinkBackStage::startExe()
{
    connect(_p->nodeProc,&QProcess::stateChanged,this,&LinkBackStage::onNodeExeStateChanged);

    QStringList strList;
    strList << "--data-dir=" + ChainIDE::getInstance()->getConfigAppDataPath().replace("\\","/")+_p->dataPath
            << QString("--rpc-endpoint=127.0.0.1:%1").arg(_p->nodePort);

    qDebug() << "start lnk_node.exe " << strList;
    _p->nodeProc->start(QCoreApplication::applicationDirPath()+QDir::separator()+DataDefine::LINK_NODE_EXE,strList);

}

bool LinkBackStage::exeRunning()
{
    return _p->clientProc->state() == QProcess::Running && _p->nodeProc->state() == QProcess::Running;
}

QProcess *LinkBackStage::getProcess() const
{
    return _p->clientProc;
}

void LinkBackStage::ReadyClose()
{
    disconnect(_p->clientProc,&QProcess::stateChanged,this,&LinkBackStage::onClientExeStateChanged);
    disconnect(_p->nodeProc,&QProcess::stateChanged,this,&LinkBackStage::onNodeExeStateChanged);
    //先lock
    rpcPostedSlot("id-lock-onCloseIDE",IDEUtil::toJsonFormat( "lock", QJsonArray()));
    rpcPostedSlot( "id-witness_node_stop",IDEUtil::toJsonFormat( "witness_node_stop", QJsonArray()));

    IDEUtil::msecSleep(5000);
    _p->clientProc->close();
    _p->nodeProc->close();
}

void LinkBackStage::onNodeExeStateChanged()
{
    if(_p->nodeProc->state() == QProcess::Starting)
    {
        qDebug() << QString("%1 is starting").arg("lnk_node.exe");
    }
    else if(_p->nodeProc->state() == QProcess::Running)
    {
        qDebug() << QString("%1 is running").arg("lnk_node.exe");
        connect(&_p->timerForStartExe,&QTimer::timeout,this,&LinkBackStage::checkNodeExeIsReady);
        _p->timerForStartExe.start(1000);
    }
    else if(_p->nodeProc->state() == QProcess::NotRunning)
    {
        CommonDialog commonDialog(CommonDialog::OkOnly);
        commonDialog.setText(tr("Fail to launch %1 !").arg("lnk_node.exe"));
        commonDialog.pop();
    }
}

void LinkBackStage::checkNodeExeIsReady()
{
    QString str = _p->nodeProc->readAllStandardError();
    qDebug() << "node exe standardError: " << str ;
    if(str.contains("Chain ID is"))
    {
        _p->timerForStartExe.stop();
        QTimer::singleShot(1000,this,&LinkBackStage::delayedLaunchClient);
    }
}

void LinkBackStage::delayedLaunchClient()
{
    connect(_p->clientProc,&QProcess::stateChanged,this,&LinkBackStage::onClientExeStateChanged);

    QStringList strList;
    strList << "--wallet-file=" + ChainIDE::getInstance()->getConfigAppDataPath().replace("\\","/") +_p->dataPath+ "/wallet.json"
            << QString("--server-rpc-endpoint=ws://127.0.0.1:%1").arg(_p->nodePort)
            << QString("--rpc-endpoint=127.0.0.1:%1").arg(_p->clientPort);

    _p->clientProc->start(QCoreApplication::applicationDirPath()+QDir::separator()+DataDefine::LINK_CLIENT_EXE,strList);
}

void LinkBackStage::onClientExeStateChanged()
{
    if(_p->clientProc->state() == QProcess::Starting)
    {
        qDebug() << QString("%1 is starting").arg("lnk_client.exe");
    }
    else if(_p->clientProc->state() == QProcess::Running)
    {
        qDebug() << QString("%1 is running").arg("lnk_client.exe");

        qDebug()<<QString("start socket connected");

        initSocketManager();
    }
    else if(_p->clientProc->state() == QProcess::NotRunning)
    {
        qDebug() << "client not running" + _p->clientProc->errorString();
        CommonDialog commonDialog(CommonDialog::OkOnly);
        commonDialog.setText(tr("Fail to launch %1 !").arg("lnk_client.exe"));
        commonDialog.pop();
    }
}

void LinkBackStage::initSocketManager()
{
    connect(_p->dataRequire,&DataRequireManager::requireResponse,ChainIDE::getInstance(),&ChainIDE::jsonDataUpdated);
    connect(_p->dataRequire,&DataRequireManager::connectFinish,this,&BackStageBase::exeStarted);

    _p->dataRequire->startManager(DataRequireManager::WEBSOCKET);
}

void LinkBackStage::rpcPostedSlot(const QString & id, const QString & cmd)
{
    _p->dataRequire->requirePosted(id,cmd);
}
