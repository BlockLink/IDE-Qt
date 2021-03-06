#ifndef CONTRACTWIDGET_H
#define CONTRACTWIDGET_H

#include <QWidget>

namespace Ui {
class ContractWidget;
}
class QTreeWidgetItem;
class ContractWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ContractWidget(QWidget *parent = 0);
    ~ContractWidget();
public slots:
    void RefreshTree();
    void retranslator();
private slots:
    void ContractClicked(QTreeWidgetItem *item, QTreeWidgetItem *itempre);
private:
    void InitWidget();
private:
    Ui::ContractWidget *ui;
};

#endif // CONTRACTWIDGET_H
