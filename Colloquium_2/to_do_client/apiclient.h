#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>

class ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);
    ~ApiClient();


    void getAllTasks();
    void getTask(int id);
    void createTask(const QString &title, const QString &description, const QString &status = "todo");
    void updateTask(int id, const QString &title, const QString &description, const QString &status);
    void updateTaskStatus(int id, const QString &status);
    void deleteTask(int id);

    void setAuthToken(const QString &token);
    void setBaseUrl(const QString &url);

signals:

    void tasksReceived(const QJsonArray &tasks);
    void taskReceived(const QJsonObject &task);
    void taskCreated(const QJsonObject &task);
    void taskUpdated(const QJsonObject &task);
    void taskDeleted(int id);
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    QString authToken = "gerestal";
    QString baseUrl = "http://localhost:8080";

    QNetworkRequest createRequest(const QString &endpoint);
    void handleError(QNetworkReply *reply);
};

#endif // APICLIENT_H
