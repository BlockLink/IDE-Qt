﻿#include "registercontractdialog.h"
#include "ui_registercontractdialog.h"

#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
#include <QPainter>
#include <QMouseEvent>
#include <QDesktopWidget>
#include <QCoreApplication>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStyledItemDelegate>
#include <QTimer>

#include "ChainIDE.h"
#include "IDEUtil.h"
#include "DataDefine.h"
#include "DataManager.h"

#include "commondialog.h"
#include "ConvenientOp.h"

RegisterContractDialog::RegisterContractDialog(QWidget *parent) :
    MoveableDialog(parent),
    ui(new Ui::RegisterContractDialog)
{
    ui->setupUi(this);

    InitWidget();
}

RegisterContractDialog::~RegisterContractDialog()
{
    delete ui;
}

void RegisterContractDialog::jsonDataUpdated(const QString &id,const QString &data)
{
    if("register-createcontract" == id)
    {
        if(!data.isEmpty() && !data.startsWith("Error"))
        {
            //获取合约地址
            ChainIDE::getInstance()->postRPC("register-getcreatecontractaddress",IDEUtil::toJsonFormat("getcreatecontractaddress",
                                              QJsonArray()<<data));

            ChainIDE::getInstance()->postRPC("register-sendrawtransaction",IDEUtil::toJsonFormat("sendrawtransaction",
                                             QJsonArray()<<data));


        }
        else
        {
            ConvenientOp::ShowSyncCommonDialog(data);
        }
    }
    else if("register-getcreatecontractaddress" == id)
    {
        //保存合约
        QJsonParseError json_error;
        QJsonDocument parse_doucment = QJsonDocument::fromJson(data.toLatin1(),&json_error);
        if(json_error.error != QJsonParseError::NoError || !parse_doucment.isObject())
        {
            ConvenientOp::ShowSyncCommonDialog(data);
            contractAddress.clear();
            return;
        }
        contractAddress = parse_doucment.object().value("address").toString();

        qDebug()<<contractAddress;

    }
    else if("register-sendrawtransaction" == id)
    {
        if(!data.startsWith("Error") && !data.isEmpty())
        {//储存合约地址
            //写入合约文件
            ConvenientOp::AddContract(ui->address->currentText(), contractAddress);

            //产一个块来确认
            ChainIDE::getInstance()->postRPC("generate",IDEUtil::toJsonFormat("generate",QJsonArray()<<1));
        }
        ConvenientOp::ShowSyncCommonDialog(data);
        close();
    }

}

void RegisterContractDialog::on_okBtn_clicked()
{
    //打开合约文件
    QString contractHex;
    QFile file(ui->contractFile->currentData().toString());
    if( file.open(QIODevice::ReadOnly))
    {
        QByteArray ba = file.readAll();
        contractHex = ba.toHex();
    }
    file.close();
    //获取注册地址
    QString registerAddr = ui->address->currentText();

    ChainIDE::getInstance()->postRPC("register-createcontract",IDEUtil::toJsonFormat("createcontract",
                                     QJsonArray()<<registerAddr<<contractHex<<ui->gaslimit->value()<<
                                     ui->gasprice->value()<<QString::number(ui->fee->value())));

}

void RegisterContractDialog::on_cancelBtn_clicked()
{
    close();
}

void RegisterContractDialog::on_closeBtn_clicked()
{
    close();
}

void RegisterContractDialog::InitWidget()
{
    contractAddress = "";
    ui->gaslimit->setRange(0,999999);
    ui->gaslimit->setSingleStep(1);
    ui->gasprice->setRange(10,999999);
    ui->fee->setRange(0,999999999999);
    ui->fee->setDecimals(8);
    ui->fee->setSingleStep(0.001);

    QStyledItemDelegate* itemDelegate = new QStyledItemDelegate();
    ui->contractFile->setItemDelegate(itemDelegate);

    setWindowFlags(Qt::FramelessWindowHint);

    //初始化combobox,读取所有.gpc文件
    QStringList fileList;
    IDEUtil::GetAllFile(QCoreApplication::applicationDirPath()+QDir::separator()+DataDefine::CONTRACT_DIR,
                        fileList,QStringList()<<DataDefine::CONTRACT_SUFFIX);
    foreach (QString file, fileList) {
        ui->contractFile->addItem(file.mid(QString(QCoreApplication::applicationDirPath()+QDir::separator()+DataDefine::CONTRACT_DIR).length()),
                                  QVariant::fromValue(file));
    }

    //读取所有账户信息
    connect(ChainIDE::getInstance()->getDataManager(),&DataManager::queryAccountFinish,this,&RegisterContractDialog::InitAccountAddress);
    ChainIDE::getInstance()->getDataManager()->queryAccount();

}

void RegisterContractDialog::InitAccountAddress()
{
    ChainIDE::getInstance()->getDataManager()->getAccount();

    QTreeWidget *tree = new QTreeWidget(this);
    tree->header()->setVisible(false);

    DataDefine::AccountDataPtr data = ChainIDE::getInstance()->getDataManager()->getAccount();
    int number = 0;
    QString currentText;
    for(auto it = data->getAccount().begin();it != data->getAccount().end();++it)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList()<<(*it)->getAccountName());
        item->setFlags(Qt::ItemIsEnabled);
        tree->addTopLevelItem(item);

        for(auto add = (*it)->getAddressInfos().begin();add != (*it)->getAddressInfos().end();++add)
        {
            QTreeWidgetItem *childitem = new QTreeWidgetItem(QStringList()<<(*add)->GetAddress());
            item->addChild(childitem);
            if(0 == number)
            {
                currentText = (*add)->GetAddress();
            }
            ++number;
        }
    }
    tree->expandAll();
    ui->address->setModel(tree->model());
    ui->address->setView(tree);


    connect(ChainIDE::getInstance(),&ChainIDE::jsonDataUpdated,this,&RegisterContractDialog::jsonDataUpdated);
}

