#include "DahuaCameraControl.h"
#include <QUrl>
#include <QDebug>
#include <QNetworkRequest>

DahuaCameraControl::DahuaCameraControl(QObject* parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::authenticationRequired, this, &DahuaCameraControl::onAuthenticationRequired);
    connect(manager, &QNetworkAccessManager::finished, this, &DahuaCameraControl::onReplyFinished);
    
    port = 80;
}

DahuaCameraControl::~DahuaCameraControl()
{
}

void DahuaCameraControl::setConnectionDetails(const QString& ip, int port, const QString& user, const QString& pass)
{
    this->ip = ip;
    this->port = port;
    this->username = user;
    this->password = pass;
}

void DahuaCameraControl::onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator)
{
    Q_UNUSED(reply);
    authenticator->setUser(username);
    authenticator->setPassword(password);
}

void DahuaCameraControl::onReplyFinished(QNetworkReply* reply)
{
    isBusy = false; // Reset busy flag

    if (reply->error() != QNetworkReply::NoError) {
        emit logMessage(QString("Errore PTZ: %1 (Codice: %2)").arg(reply->errorString()).arg((int)reply->error()));
    } else {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit logMessage(QString("Risposta PTZ OK (Status %1)").arg(statusCode));
    }
    reply->deleteLater();
}

QString DahuaCameraControl::buildUrl(const QString& action, const QString& code, int arg1, int arg2, int arg3)
{
    // Dahua PTZ HTTP API format:
    // /cgi-bin/ptz.cgi?action=[start|stop]&channel=1&code=[Code]&arg1=0&arg2=[Speed]&arg3=0
    // OR generic: arg1=pan, arg2=tilt, arg3=zoom
    
    return QString("http://%1:%2/cgi-bin/ptz.cgi?action=%3&channel=1&code=%4&arg1=%5&arg2=%6&arg3=%7")
            .arg(ip).arg(port).arg(action).arg(code).arg(arg1).arg(arg2).arg(arg3);
}

void DahuaCameraControl::sendPtzCommand(const QString& action, const QString& code, int speed)
{
    if (ip.isEmpty()) {
        emit logMessage("PTZ Error: IP is empty!");
        return;
    }
    
    // Throttling: Ignore if busy
    // if (isBusy) {
        // DISABLE BUSY CHECK FOR DUAL COMMANDS
        // return; 
    // }

    // Map old single-axis command to Vector (Assuming Pan/Tilt logic or Generic)
    // Legacy logic: arg2 was speed (for Pan/Tilt actions).
    // If code is Up/Down/Left/Right, usually arg2 is speed. Arg1 is 0.
    // If Zoom, arg2 is speed.
    // So mapping (0, speed, 0) preserves old behavior.
    QString urlStr = buildUrl(action, code, 0, speed, 0);
    QUrl url(urlStr);
    
    emit logMessage("Richiesta PTZ: " + urlStr);

    QNetworkRequest request(url);
    // Timeout of 2500ms (2.5 seconds) to prevent freezing
    request.setTransferTimeout(2500); 
    
    isBusy = true; // Set busy flag
    manager->get(request);
}

void DahuaCameraControl::stop()
{
    sendPtzCommand("stop", "Up", 0);
}

void DahuaCameraControl::sendCustomPtz(const QString& action, const QString& code, int arg1, int arg2, int arg3)
{
    if (ip.isEmpty()) {
        emit logMessage("PTZ Error: IP is empty!");
        return;
    }
    
    // Throttling: Ignore if busy (Maybe disable for continuous too? No, safer to keep unless issues)
    // if (isBusy) return; // Keeping disabled from previous step or re-enabling?
    // Let's Keep disabled for responsiveness

    QString urlStr = buildUrl(action, code, arg1, arg2, arg3);
    QUrl url(urlStr);
    
    emit logMessage("Richiesta Custom PTZ: " + urlStr);

    QNetworkRequest request(url);
    request.setTransferTimeout(2500); 
    
    isBusy = true; 
    manager->get(request);
}
