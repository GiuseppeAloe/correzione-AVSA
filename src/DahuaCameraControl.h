#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QString>

class DahuaCameraControl : public QObject
{
    Q_OBJECT
public:
    explicit DahuaCameraControl(QObject *parent = nullptr);
    ~DahuaCameraControl();

    void setConnectionDetails(const QString& ip, int port, const QString& user, const QString& pass);
    
    // Legacy single method (remapped internally)
    void sendPtzCommand(const QString& action, const QString& code, int speed);
    
    // New Vector/Continuous method
    void sendCustomPtz(const QString& action, const QString& code, int arg1, int arg2, int arg3);
    
    // Stop all (helper)
    void stop();

signals:
    void logMessage(const QString& msg);

private slots:
    void onReplyFinished(QNetworkReply* reply);
    void onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator);

private:
    QString buildUrl(const QString& action, const QString& code, int arg1, int arg2, int arg3);

    QNetworkAccessManager* manager;
    QString ip;
    int port;
    QString username;
    QString password;
    bool isBusy = false;
};
