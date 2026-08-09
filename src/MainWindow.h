#include <QSlider>
#pragma once





#include <QMainWindow>


#include <QDoubleSpinBox>


#include <QSpinBox>


#include <QGroupBox>


#include <QFormLayout>


#include <QImage>


#include <QGraphicsScene>


#include <QElapsedTimer>


#include <vector>





class LiveFrameProcessor;


class QLineEdit;


class QGraphicsView;





#include <QGraphicsScene>
#include <QPolygonF>
#include <QSize>
#include <QString>
#include <QImage>

class QGraphicsPolygonItem;
class QGraphicsPixmapItem;

#include "RoiScene.h"




class QPushButton;


class QProgressBar;


class QLabel;


class QCheckBox;


class QComboBox;


class QGroupBox;


class QSpinBox;


struct DetectionParams;


class QRadioButton;
class DahuaCameraControl;





class MainWindow : public QMainWindow


{


    Q_OBJECT





public:


    explicit MainWindow(QWidget* parent = nullptr);


    ~MainWindow() override;





private slots:
    void onPrimaryObjectDetected(QRectF rect, QPointF center, QSize frameSize);
    void onAutoPtzWatchdog();


    // UI Slots


    void onBrowseInput();


    void onBrowseOutputDir();


    


    void onLoadFirstFrame();


    void onStart();


    void onCancel();


    void onReset();


    void onSourceChanged();
    void onConnectRtsp(); 


    


    // Config


    void onToggleRoiEdit(bool on);


    void onPresetChanged(int index);


    void onRoiPolygonChanged(const QPolygonF& poly);





    // Worker Slots


    void onLiveFrame(const QImage& img);


    void onProgress(int percent);


    void onFinished();


    void onWorkerError(const QString& msg);


    void onElapsedTime(const QString& timeStr, qint64 ms);
    void onVideoInfoReady(int width, int height, double fps, int totalFrames, QString codec);





    // Menu


    void onActionHelp();


    void onActionInfo();





private:
    bool startingPreview_ = false;


    void createMenu();


    void startCurrentFile();
    bool hasMoreFiles() const;

    void advanceToNextFile(); 
    void updateAutoOutputNames();


    


    void uiFromParams(const DetectionParams& p);


    DetectionParams paramsFromUi() const;







    // Widgets


    QLineEdit* inEdit_ = nullptr;


    QLineEdit* outDirEdit_ = nullptr;


    


    // RTSP Controls


    QRadioButton* rbSourceFile_ = nullptr;


    QRadioButton* rbSourceNet_ = nullptr;


    QLineEdit* leRtspUrl_ = nullptr;


    QPushButton* btnConnectRtsp_ = nullptr;





    QLineEdit* outVideoEdit_ = nullptr;


    QLineEdit* outCsvEdit_ = nullptr;





    QPushButton* btnLoadFrame_ = nullptr;


    QCheckBox* cbUseRoi_ = nullptr;


    QCheckBox* cbEditRoi_ = nullptr;


    QCheckBox* cbDrawRoi_ = nullptr;


    


    // New Controls


    QCheckBox* cbNoPreview_ = nullptr;


    QCheckBox* cbBlur_ = nullptr;


    QCheckBox* cbUseCuda_ = nullptr;


    QSpinBox*  sbCpuThreads_ = nullptr;





    QCheckBox* cbClipOnly_ = nullptr;


    QCheckBox* cbSaveSnapshots_ = nullptr; 


    QLabel*    lblTimer_   = nullptr;
    QLabel*    lblTotalTimer_ = nullptr; // New
    qint64     totalBatchTimeMs_ = 0;
    qint64     lastReportedMs_ = 0;


    


    QElapsedTimer batchTimer_;





    // Detection Config


    QComboBox* cbPreset_ = nullptr;


    


    // Group Manual


    QGroupBox* grpManual_ = nullptr;


    QSpinBox* sbSens_ = nullptr;


    QSpinBox* sbProcEvery_ = nullptr;


    QSpinBox* sbMinHits_ = nullptr;


    QSpinBox* sbMinMove_ = nullptr;


    QSpinBox* sbAccFrames_ = nullptr;


    QSpinBox* sbAccMinHits_ = nullptr;





    QPushButton* btnStart_ = nullptr;


    QPushButton* btnCancel_ = nullptr;


    QPushButton* btnReset_ = nullptr;


    QProgressBar* prog_ = nullptr;
    QProgressBar* progBatch_ = nullptr; // New Batch Progress
    QLabel*       statusLbl_ = nullptr;
    QLabel*       lblFileInfo_ = nullptr; // New Info Label





    QGraphicsView* view_ = nullptr;


    RoiScene* scene_ = nullptr;





    // Logic


    QStringList inputFiles_;


    int currentIndex_ = 0;





    LiveFrameProcessor* worker_ = nullptr;

    void resetUiControls();


    QDoubleSpinBox* sbDistance_ = nullptr;


    QSpinBox* sbZoom_ = nullptr;

    // Camera Control (PTZ)
    DahuaCameraControl* ptzCtrl_ = nullptr;
    QGroupBox* grpPtz_ = nullptr;

    // Auto-Tracking Members
    QCheckBox* chkAutoPtz_ = nullptr;
    QLabel* lblTrackingStatus_ = nullptr;
    QTimer* autoPtzWatchdog_ = nullptr;
    int autoPtzState_ = 0; // 0=Idle, 1=Left, 2=Right, 3=Up, 4=Down
    int noDetectCounter_ = 0;
    QPointF lastTargetPos_;

    QSlider* ptzSpeedSlider_ = nullptr;
    QLineEdit* leCamUser_ = nullptr;
    QLineEdit* leCamPass_ = nullptr;
    QLineEdit* leCamIp_ = nullptr;
    
    // PTZ Buttons
    QPushButton* btnPtzUp_ = nullptr;
    QPushButton* btnPtzDown_ = nullptr;
    QPushButton* btnPtzLeft_ = nullptr;
    QPushButton* btnPtzRight_ = nullptr;
    QPushButton* btnPtzUpLeft_ = nullptr;
    QPushButton* btnPtzUpRight_ = nullptr;
    QPushButton* btnPtzDownLeft_ = nullptr;
    QPushButton* btnPtzDownRight_ = nullptr;
    QPushButton* btnZoomIn_ = nullptr;
    QPushButton* btnZoomOut_ = nullptr;
    QPushButton* btnFocusNear_ = nullptr;
    QPushButton* btnFocusFar_ = nullptr;
    
    // Step Control
    QTimer* ptzStepTimer_ = nullptr;

private slots:
    void onPtzStepFinished();


};




