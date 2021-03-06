#ifndef EXEMANAGER_H
#define EXEMANAGER_H

#include <QObject>
//管理exe的启动、websocket的链接，完成后发出startExe信号
#include <QProcess>
class ExeManager : public QObject
{
    Q_OBJECT
public:
    explicit ExeManager(int type = 1,QObject *parent = 0);//链类型1==测试 2==正式
    ~ExeManager();

public:
    void startExe();
    bool exeRunning();
    QProcess *getProcess()const;
signals:
    void exeStarted();
private slots:
    void onNodeExeStateChanged();
    void checkNodeExeIsReady();
    void delayedLaunchClient();
    void onClientExeStateChanged();

public slots:
    void rpcPostedSlot(const QString &,const QString &);
private:
    void initSocketManager();
private:
    class DataPrivate;
    DataPrivate *_p;
};

#endif // EXEMANAGER_H
