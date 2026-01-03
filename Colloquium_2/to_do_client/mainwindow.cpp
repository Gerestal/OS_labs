#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QHeaderView>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    ui->tasksTable->setColumnCount(4);
    QStringList headers = {"ID", "Title", "Description", "Status"};
    ui->tasksTable->setHorizontalHeaderLabels(headers);

    for (int i = 0; i < ui->tasksTable->columnCount(); ++i) {
        ui->tasksTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }


    ui->tasksTable->horizontalHeader()->setStretchLastSection(false);



    ui->statusCombo->addItems({"todo", "in_progress", "done"});
    ui->statusComboEdit->addItems({"todo", "in_progress", "done"});


    networkManager = new QNetworkAccessManager(this);


    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(ui->createButton, &QPushButton::clicked, this, &MainWindow::onCreateClicked);
    connect(ui->updateButton, &QPushButton::clicked, this, &MainWindow::onUpdateClicked);
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);


    connect(ui->tasksTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onTaskSelected);


    onRefreshClicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::onTaskSelected()
{
    int selectedRow = ui->tasksTable->currentRow();
    if (selectedRow >= 0) {

        currentTaskId = ui->tasksTable->item(selectedRow, 0)->text().toInt();


        currentTitle = ui->tasksTable->item(selectedRow, 1)->text();
        currentDescription = ui->tasksTable->item(selectedRow, 2)->text();
        currentStatus = ui->tasksTable->item(selectedRow, 3)->text();


        ui->titleEditEdit->setText(currentTitle);
        ui->descEditEdit->setPlainText(currentDescription);


        int index = ui->statusComboEdit->findText(currentStatus);
        if (index >= 0) {
            ui->statusComboEdit->setCurrentIndex(index);
        }

        taskLoadedForEdit = true;
        ui->statusLabel->setText(QString("Task #%1 selected").arg(currentTaskId));
    }
}


void MainWindow::onRefreshClicked()
{
    QNetworkRequest request(QUrl("http://localhost:8080/tasks"));
    request.setRawHeader("Authorization", "gerestal");
    networkManager->get(request);
    ui->statusLabel->setText("Loading tasks...");
}


void MainWindow::onCreateClicked()
{
    QString title = ui->titleEdit->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Title cannot be empty!");
        return;
    }

    QJsonObject task;
    task["title"] = title;
    task["description"] = ui->descEdit->toPlainText();
    task["status"] = ui->statusCombo->currentText();

    QNetworkRequest request(QUrl("http://localhost:8080/tasks"));
    request.setRawHeader("Authorization", "gerestal");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument doc(task);
    networkManager->post(request, doc.toJson());

    ui->statusLabel->setText("Creating task...");
}


void MainWindow::onUpdateClicked()
{
    if (currentTaskId == -1) {
        QMessageBox::warning(this, "Warning", "Select a task first!");
        return;
    }

    if (!taskLoadedForEdit) {
        QMessageBox::warning(this, "Warning", "Load task data first by selecting it in the table!");
        return;
    }


    QJsonObject task;


    QString newTitle = ui->titleEditEdit->text().trimmed();
    if (!newTitle.isEmpty()) {
        task["title"] = newTitle;
    } else {
        task["title"] = currentTitle;
    }

    QString newDescription = ui->descEditEdit->toPlainText().trimmed();
    if (!newDescription.isEmpty()) {
        task["description"] = newDescription;
    } else {
        task["description"] = currentDescription;
    }


    task["status"] = ui->statusComboEdit->currentText();

    QNetworkRequest request(QUrl("http://localhost:8080/tasks/" + QString::number(currentTaskId)));
    request.setRawHeader("Authorization", "gerestal");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument doc(task);
    networkManager->put(request, doc.toJson());

    ui->statusLabel->setText("Updating task...");
}


void MainWindow::onDeleteClicked()
{
    if (currentTaskId == -1) {
        QMessageBox::warning(this, "Warning", "Select a task first!");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Delete Task",
                                  QString("Are you sure you want to delete task #%1?").arg(currentTaskId),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QNetworkRequest request(QUrl("http://localhost:8080/tasks/" + QString::number(currentTaskId)));
        request.setRawHeader("Authorization", "gerestal");
        networkManager->deleteResource(request);

        ui->statusLabel->setText("Deleting task...");
    }
}


void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        ui->statusLabel->setText("Error: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray response = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response);

    QString url = reply->url().toString();

    if (url.contains("/tasks/") && reply->operation() == QNetworkAccessManager::GetOperation) {

        if (jsonDoc.isObject()) {
            QJsonObject task = jsonDoc.object();


            currentTaskId = task["id"].toInt();
            currentTitle = task["title"].toString();
            currentDescription = task["description"].toString();
            currentStatus = task["status"].toString();


            ui->titleEditEdit->setText(currentTitle);
            ui->descEditEdit->setPlainText(currentDescription);

            QString status = task["status"].toString();
            int index = ui->statusComboEdit->findText(status);
            if (index >= 0) ui->statusComboEdit->setCurrentIndex(index);

            taskLoadedForEdit = true;
            ui->statusLabel->setText(QString("Task #%1 loaded").arg(currentTaskId));
        }
    }
    else if (url.endsWith("/tasks") && reply->operation() == QNetworkAccessManager::GetOperation) {

        columnWidths.clear();
        for (int i = 0; i < ui->tasksTable->columnCount(); ++i) {
            columnWidths.append(ui->tasksTable->columnWidth(i));
        }

        if (jsonDoc.isArray()) {
            QJsonArray tasks = jsonDoc.array();
            ui->tasksTable->setRowCount(0);

            for (const QJsonValue &value : tasks) {
                QJsonObject task = value.toObject();
                int row = ui->tasksTable->rowCount();
                ui->tasksTable->insertRow(row);

                ui->tasksTable->setItem(row, 0, new QTableWidgetItem(QString::number(task["id"].toInt())));
                ui->tasksTable->setItem(row, 1, new QTableWidgetItem(task["title"].toString()));
                ui->tasksTable->setItem(row, 2, new QTableWidgetItem(task["description"].toString()));
                ui->tasksTable->setItem(row, 3, new QTableWidgetItem(task["status"].toString()));
            }

            ui->statusLabel->setText(QString("Loaded %1 tasks").arg(tasks.size()));
            if (!columnWidths.isEmpty()) {
                for (int i = 0; i < qMin(columnWidths.size(), ui->tasksTable->columnCount()); ++i) {
                    ui->tasksTable->setColumnWidth(i, columnWidths[i]);
                }
            }
        }
    }
    else if (reply->operation() == QNetworkAccessManager::PostOperation) {

        ui->titleEdit->clear();
        ui->descEdit->clear();
        ui->statusCombo->setCurrentIndex(0);
        onRefreshClicked();
        ui->statusLabel->setText("Task created successfully!");
    }
    else if (reply->operation() == QNetworkAccessManager::PutOperation) {

        onRefreshClicked();

        if (!columnWidths.isEmpty()) {
            for (int i = 0; i < qMin(columnWidths.size(), ui->tasksTable->columnCount()); ++i) {
                ui->tasksTable->setColumnWidth(i, columnWidths[i]);
            }
        }

        if (jsonDoc.isObject()) {
            QJsonObject task = jsonDoc.object();
            currentTitle = task["title"].toString();
            currentDescription = task["description"].toString();
            currentStatus = task["status"].toString();
        }

        ui->statusLabel->setText("Task updated successfully!");
    }
    else if (reply->operation() == QNetworkAccessManager::DeleteOperation) {

        currentTaskId = -1;
        currentTitle.clear();
        currentDescription.clear();
        currentStatus.clear();
        taskLoadedForEdit = false;


        ui->titleEditEdit->clear();
        ui->descEditEdit->clear();
        ui->statusComboEdit->setCurrentIndex(0);

        onRefreshClicked();
        ui->statusLabel->setText("Task deleted successfully!");
    }

    reply->deleteLater();
}
