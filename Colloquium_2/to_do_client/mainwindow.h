#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRefreshClicked();
    void onCreateClicked();
    void onUpdateClicked();
    void onDeleteClicked();
    void onNetworkReply(QNetworkReply *reply);

    void onTaskSelected();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;


    int currentTaskId = -1;
    QString currentTitle;
    QString currentDescription;
    QString currentStatus;


    bool taskLoadedForEdit = false;

     QVector<int> columnWidths;
};

#endif // MAINWINDOW_H
