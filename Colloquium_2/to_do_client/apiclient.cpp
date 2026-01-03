#include "apiclient.h"
#include <QNetworkRequest>
#include <QJsonArray>
#include <QDebug>
#include <QBuffer>

ApiClient::ApiClient(QObject *parent) : QObject(parent)
{
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &ApiClient::onReplyFinished);
}

ApiClient::~ApiClient()
{
    delete networkManager;
}

void ApiClient::setAuthToken(const QString &token)
{
    authToken = token;
}

void ApiClient::setBaseUrl(const QString &url)
{
    baseUrl = url;
}

QNetworkRequest ApiClient::createRequest(const QString &endpoint)
{
    QUrl url(baseUrl + endpoint);
    QNetworkRequest request(url);

    request.setRawHeader("Authorization", authToken.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    return request;
}

void ApiClient::getAllTasks()
{
    QNetworkRequest request = createRequest("/tasks");
    networkManager->get(request);
}

void ApiClient::getTask(int id)
{
    QNetworkRequest request = createRequest(QString("/tasks/%1").arg(id));
    networkManager->get(request);
}

void ApiClient::createTask(const QString &title, const QString &description, const QString &status)
{
    QNetworkRequest request = createRequest("/tasks");

    QJsonObject json;
    json["title"] = title;
    json["description"] = description;
    json["status"] = status;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    networkManager->post(request, data);
}

void ApiClient::updateTask(int id, const QString &title, const QString &description, const QString &status)
{
    QNetworkRequest request = createRequest(QString("/tasks/%1").arg(id));

    QJsonObject json;
    json["title"] = title;
    json["description"] = description;
    json["status"] = status;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    networkManager->put(request, data);
}

void ApiClient::updateTaskStatus(int id, const QString &status)
{
    QNetworkRequest request = createRequest(QString("/tasks/%1").arg(id));

    QJsonObject json;
    json["status"] = status;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();


    QBuffer *buffer = new QBuffer();
    buffer->setData(data);
    buffer->open(QIODevice::ReadOnly);

    QNetworkReply *reply = networkManager->sendCustomRequest(request, "PATCH", buffer);
    buffer->setParent(reply);
}

void ApiClient::deleteTask(int id)
{
    QNetworkRequest request = createRequest(QString("/tasks/%1").arg(id));
    networkManager->deleteResource(request);
}

void ApiClient::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        handleError(reply);
        reply->deleteLater();
        return;
    }

    QByteArray response = reply->readAll();
    QString endpoint = reply->url().path();


    QNetworkAccessManager::Operation operation = reply->operation();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(response);

    if (operation == QNetworkAccessManager::GetOperation) {
        if (endpoint.startsWith("/tasks/")) {

            if (jsonDoc.isObject()) {
                emit taskReceived(jsonDoc.object());
            }
        } else if (endpoint == "/tasks") {

            if (jsonDoc.isArray()) {
                emit tasksReceived(jsonDoc.array());
            }
        }
    }
    else if (operation == QNetworkAccessManager::PostOperation) {
        if (jsonDoc.isObject()) {
            emit taskCreated(jsonDoc.object());
        }
    }
    else if (operation == QNetworkAccessManager::PutOperation) {
        if (jsonDoc.isObject()) {
            emit taskUpdated(jsonDoc.object());
        }
    }
    else if (operation == QNetworkAccessManager::CustomOperation) {

        if (jsonDoc.isObject()) {
            emit taskUpdated(jsonDoc.object());
        }
    }
    else if (operation == QNetworkAccessManager::DeleteOperation) {

        QString path = reply->url().path();
        QStringList parts = path.split('/');
        if (parts.size() >= 3) {
            bool ok;
            int id = parts[2].toInt(&ok);
            if (ok) emit taskDeleted(id);
        }
    }

    reply->deleteLater();
}

void ApiClient::handleError(QNetworkReply *reply)
{
    QString errorStr = QString("Error %1: %2")
    .arg(reply->error())
        .arg(reply->errorString());

    qDebug() << "API Error:" << errorStr;
    emit errorOccurred(errorStr);
}
