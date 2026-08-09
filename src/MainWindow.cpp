

#include "RoiScene.h"
#include <QGraphicsScene>
#include <QDebug>
#include <QApplication>
#include <QSettings>
#include <QCoreApplication>
#include "DahuaCameraControl.h"
#include "MainWindow.h"
#include "LiveFrameProcessor.h"


#include <thread>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <windows.h>






#include <QVBoxLayout>
#include <QSplitter>
#include <QScrollArea>


#include <QHBoxLayout>


#include <QPushButton>


#include <QLineEdit>


#include <QLabel>


#include <QCheckBox>


#include <QProgressBar>


#include <QGraphicsView>


#include <QSpinBox>


#include <QFileDialog>


#include <QFileInfo>


#include <QDir>


#include <QMessageBox>


#include <QComboBox>


#include <QGroupBox>


#include <QFormLayout>


#include <QMenuBar>


#include <QMenu>


#include <QAction>


#include <QTextBrowser>


#include <QDialog>


#include <QVBoxLayout>
#include <QSplitter>
#include <QScrollArea>


#include <QDialogButtonBox>


#include <QRadioButton> // NEW


#include <QDateTime> // NEW
#include <QTimer>


#include "DetectionPreset.h"





static QString makeProgressiveName(


    const QString& dir,


    const QString& base,


    const QString& suffix,


    const QString& ext)


{


    QDir d(dir);


    QString name = base + suffix + "." + ext;


    if (!d.exists(name))


        return d.filePath(name);





    int idx = 1;


    while (true)


    {


        QString c = base + suffix + "_" + QString::number(idx) + "." + ext;


        if (!d.exists(c))


            return d.filePath(c);


        ++idx;


    }


}





MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) { 
    std::cout << "DEBUG: MW Ctor Start" << std::endl;
    // AllocConsole();
    // freopen("CONOUT$", "w", stdout);
    // freopen("CONOUT$", "w", stderr);
    
    qDebug() << "MainWindow CONSTRUCTOR Called";
    std::cout << "DEBUG: === BUILD VERSION 2.0 (Checkpoints Active) ===" << std::endl;
    std::cout << "DEBUG: MW Qt Plugins Loaded" << std::endl;
    
    std::cout << "DEBUG: Lib Paths: " << QCoreApplication::libraryPaths().join(", ").toStdString() << std::endl;

    std::cout << "DEBUG: Creating central widget..." << std::endl;
    QWidget* central = new QWidget(this);
    std::cout << "DEBUG: Central widget created." << std::endl;

    std::cout << "DEBUG: Setting central widget..." << std::endl;
    setCentralWidget(central);
    std::cout << "DEBUG: Central widget set." << std::endl;
    
    std::cout << "DEBUG: Resizing..." << std::endl;
    resize(1600, 900);
    setMinimumSize(1024, 768);
    // resize(1600, 900); // Removed duplicate
    // setMinimumSize(1024, 768); // Removed duplicate
    std::cout << "DEBUG: Resize done." << std::endl;


    std::cout << "DEBUG: Calling createMenu()..." << std::endl;
    createMenu();
    std::cout << "DEBUG: createMenu() returned." << std::endl;





    // REFACTOR: Elastic Layout using QSplitter
    QHBoxLayout* mainWrapper = new QHBoxLayout(central);
    mainWrapper->setContentsMargins(0,0,0,0);
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal);
    mainWrapper->addWidget(mainSplitter);


    // Scroll Area added
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumWidth(320); // Elastic Min Width
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* scrollContent = new QWidget();
    // Right Layout for PTZ
    QVBoxLayout* right = new QVBoxLayout();

    // Left Layout (Analysis)
    QVBoxLayout* left = new QVBoxLayout(scrollContent);
    scrollArea->setWidget(scrollContent);


    mainSplitter->addWidget(scrollArea);






    // ================= SOURCE SELECT =================
    // SKIP QGroupBox to fix crash
    // QGroupBox* grpSource = new QGroupBox("Sorgente Input");
    std::cout << "DEBUG: Skipping grpSource creation to fix crash." << std::endl;

    // Create standalone layout
    QVBoxLayout* srcLay = new QVBoxLayout(); 
    std::cout << "DEBUG: Standalone srcLay created." << std::endl;
    std::cout << "DEBUG: BoxLayout created. Creating RadioButtons..." << std::endl;
    
    // Radio Buttons
    QHBoxLayout* radioLay = new QHBoxLayout();
    rbSourceFile_ = new QRadioButton("File Video");
    std::cout << "DEBUG: rbSourceFile created." << std::endl;

    rbSourceNet_  = new QRadioButton("Flusso Rete (RTSP/Live)");
    std::cout << "DEBUG: rbSourceNet created." << std::endl;

    rbSourceFile_->setChecked(true); // Default
    radioLay->addWidget(rbSourceFile_);
    radioLay->addWidget(rbSourceNet_);
    srcLay->addLayout(radioLay);
    std::cout << "DEBUG: Radio Layout added." << std::endl;









    // File Input Widgets
    std::cout << "DEBUG: Creating inEdit_ (QLineEdit)..." << std::endl;
    inEdit_ = new QLineEdit();
    std::cout << "DEBUG: inEdit_ created." << std::endl;

    inEdit_->setReadOnly(true);
    std::cout << "DEBUG: inEdit_ setReadOnly done." << std::endl;

    std::cout << "DEBUG: Creating btnInput (QPushButton)..." << std::endl;
    QPushButton* btnInput = new QPushButton("Scegli File...");
    btnInput->setObjectName("btnBrowseInput");
    std::cout << "DEBUG: btnInput created." << std::endl;

    connect(btnInput, &QPushButton::clicked, this, &MainWindow::onBrowseInput);
    std::cout << "DEBUG: btnInput connected." << std::endl;


    


    // Network Input Widgets
    std::cout << "DEBUG: Creating leRtspUrl_..." << std::endl;
    leRtspUrl_ = new QLineEdit("rtsp://admin:password@192.168.1.108:554/cam/realmonitor?channel=1&subtype=0");
    std::cout << "DEBUG: leRtspUrl_ created." << std::endl;

    leRtspUrl_->setPlaceholderText("rtsp://user:pass@ip:port/...");
    leRtspUrl_->setVisible(false); // Hide initially
    std::cout << "DEBUG: leRtspUrl_ configured." << std::endl;

    std::cout << "DEBUG: Creating btnConnectRtsp_..." << std::endl;
    btnConnectRtsp_ = new QPushButton("Test Connessione"); // Optional
    std::cout << "DEBUG: btnConnectRtsp_ created." << std::endl;

    btnConnectRtsp_->setVisible(false);
    connect(btnConnectRtsp_, &QPushButton::clicked, this, &MainWindow::onConnectRtsp);
    std::cout << "DEBUG: btnConnectRtsp_ configured." << std::endl;





    // Layout Source


    std::cout << "DEBUG: Creating QLabel (Input)..." << std::endl;
    QLabel* lblInput = new QLabel("Input:", this); // Added parent this just in case
    std::cout << "DEBUG: QLabel created. Adding to layout..." << std::endl;
    srcLay->addWidget(lblInput);
    std::cout << "DEBUG: Label added to layout." << std::endl;

    std::cout << "DEBUG: Adding inEdit_ to layout..." << std::endl;
    srcLay->addWidget(inEdit_);
    std::cout << "DEBUG: inEdit_ added." << std::endl;

    std::cout << "DEBUG: Adding leRtspUrl_ to layout..." << std::endl;
    srcLay->addWidget(leRtspUrl_); 
    std::cout << "DEBUG: leRtspUrl_ added." << std::endl;

    std::cout << "DEBUG: Adding btnInput to layout..." << std::endl;
    srcLay->addWidget(btnInput);   
    std::cout << "DEBUG: btnInput added." << std::endl;

    std::cout << "DEBUG: Adding btnConnectRtsp_ to layout..." << std::endl;
    srcLay->addWidget(btnConnectRtsp_);
    std::cout << "DEBUG: btnConnectRtsp_ added." << std::endl;


    


    std::cout << "DEBUG: Creating lblFileInfo_..." << std::endl;
    lblFileInfo_ = new QLabel("File Info Placeholder"); 
    std::cout << "DEBUG: lblFileInfo_ created. Adding to layout..." << std::endl;
    srcLay->addWidget(lblFileInfo_);
    std::cout << "DEBUG: lblFileInfo_ added." << std::endl;





    // left->addWidget(grpSource);
    left->addLayout(srcLay);
    std::cout << "DEBUG: srcLay added directly to left." << std::endl;


    


    // Connect Radio change


    connect(rbSourceFile_, &QRadioButton::toggled, this, &MainWindow::onSourceChanged);
    connect(rbSourceNet_, &QRadioButton::toggled, this, &MainWindow::onSourceChanged);





    // ================= OUTPUT DIR =================


    std::cout << "DEBUG: Creating outDirEdit_..." << std::endl;
    outDirEdit_ = new QLineEdit();
    std::cout << "DEBUG: outDirEdit_ created." << std::endl;


    // outDirEdit_->setReadOnly(true);
    std::cout << "DEBUG: Skipped outDirEdit_ ->setReadOnly." << std::endl;

    QPushButton* btnOut = new QPushButton("Cartella Output...");

    connect(btnOut, &QPushButton::clicked, this, &MainWindow::onBrowseOutputDir);
    // std::cout << "DEBUG: Skipped btnOut connect." << std::endl;





    left->addWidget(new QLabel("Cartella output"));


    left->addWidget(outDirEdit_);


    left->addWidget(btnOut);





    // ================= OUTPUT FILES =================


    outVideoEdit_ = new QLineEdit();


    // outVideoEdit_->setReadOnly(true);


    left->addWidget(new QLabel("Output video (corrente)"));


    left->addWidget(outVideoEdit_);





    outCsvEdit_ = new QLineEdit();


    // outCsvEdit_->setReadOnly(true);


    left->addWidget(new QLabel("Output CSV (corrente)"));


    left->addWidget(outCsvEdit_);





    // ================= FRAME =================


    btnLoadFrame_ = new QPushButton("Carica 1 Frame");


    connect(btnLoadFrame_, &QPushButton::clicked,


            this, &MainWindow::onLoadFirstFrame);


    left->addWidget(btnLoadFrame_);

    // Info Label (File Details)
    lblFileInfo_ = new QLabel("File Info: Ready");
    lblFileInfo_->setStyleSheet("color: #AAAAAA; font-size: 11px; margin-bottom: 5px;");
    left->addWidget(lblFileInfo_);








    // ================= ROI & CONFIG =================


    cbUseRoi_ = new QCheckBox("Usa ROI per analisi");
    cbUseRoi_->setToolTip("Attiva/Disattiva analisi solo nella zona ROI");


    cbEditRoi_ = new QCheckBox("Edit ROI");
    cbEditRoi_->setToolTip("Modifica i punti del poligono ROI");


    cbDrawRoi_ = new QCheckBox("Mostra ROI nel video");
    cbDrawRoi_->setToolTip("Visualizza bordi ROI a video");





    // NEW CONTROLS
    cbNoPreview_ = new QCheckBox("Disabilita Preview (piu veloce)");
    // cbNoPreview_->setText("Disabilita Preview (piu veloce)");


    cbBlur_ = new QCheckBox("Riduci Rumore (Blur)");


    


    cbUseCuda_ = new QCheckBox("Usa Accelerazione GPU (CUDA)");


    


    cbClipOnly_ = new QCheckBox("Salva solo Clip (Rilevamento)");


    cbSaveSnapshots_ = new QCheckBox("Salva Foto Rilevamenti (Full + Zoom)"); // NEW





    left->addWidget(cbUseRoi_);


    left->addWidget(cbEditRoi_);


    left->addWidget(cbDrawRoi_);


    left->addWidget(cbClipOnly_);


        left->addWidget(cbSaveSnapshots_);





    QGroupBox* grpSpeed = new QGroupBox("Stima Velocita Reale");


    QFormLayout* spdLay = new QFormLayout(grpSpeed);


    


    sbDistance_ = new QDoubleSpinBox();


    sbDistance_->setRange(0, 100000); // 0 to 100km


    sbDistance_->setSuffix(" m");


    sbDistance_->setValue(0);


    sbDistance_->setToolTip("Distanza stimata dell'oggetto (0 = Disabilitato)");


    


    sbZoom_ = new QSpinBox();


    sbZoom_->setRange(1, 25);


    sbZoom_->setSuffix("x");


    sbZoom_->setValue(1);


    sbZoom_->setToolTip("Fattore di zoom ottico utilizzato (1x - 25x)");


    


    spdLay->addRow("Distanza:", sbDistance_);


    spdLay->addRow("Zoom:", sbZoom_);


    


    left->addWidget(grpSpeed);

    // ================= PTZ CONTROL (DAHUA) =================
    std::cout << "DEBUG: Skipping grpPtz creation to fix crash." << std::endl;
    // grpPtz_ = new QGroupBox("Controlli Telecamera (PTZ)");
    // QVBoxLayout* ptzMainLay = new QVBoxLayout(grpPtz_);
    
    // Standalone layout
    QVBoxLayout* ptzMainLay = new QVBoxLayout();
    
    // Auth fields
    QFormLayout* authLay = new QFormLayout();
    leCamIp_ = new QLineEdit("192.168.88.32");
    leCamUser_ = new QLineEdit("admin");
    leCamPass_ = new QLineEdit("giuseppe1960");
    
    leCamPass_->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    
    // Load Settings
    QSettings settings("Gemini", "VideoAnalyzer");
    leCamIp_->setText(settings.value("PtzIP", "192.168.88.32").toString());
    leCamUser_->setText(settings.value("PtzUser", "admin").toString());
    leCamPass_->setText(settings.value("PtzPass", "giuseppe1960").toString());
    if (leRtspUrl_) leRtspUrl_->setText(settings.value("RtspUrl", leRtspUrl_->text()).toString());

    
    // Connect to Save Settings on change
    auto saveSettings = [this]() {
        QSettings settings("Gemini", "VideoAnalyzer");
        settings.setValue("PtzIP", leCamIp_->text());
        settings.setValue("PtzUser", leCamUser_->text());
        settings.setValue("PtzPass", leCamPass_->text());
        if (leRtspUrl_) settings.setValue("RtspUrl", leRtspUrl_->text());

    };
    connect(leCamIp_, &QLineEdit::textChanged, saveSettings);
    connect(leCamUser_, &QLineEdit::textChanged, saveSettings);
    connect(leCamPass_, &QLineEdit::textChanged, saveSettings);
    if (leRtspUrl_) connect(leRtspUrl_, &QLineEdit::textChanged, saveSettings);


    authLay->addRow("IP:", leCamIp_);
    authLay->addRow("User:", leCamUser_);
    authLay->addRow("Pass:", leCamPass_);
    ptzMainLay->addLayout(authLay);

    // Speed Slider
    ptzSpeedSlider_ = new QSlider(Qt::Horizontal);
    ptzSpeedSlider_->setRange(1, 8);
    ptzSpeedSlider_->setValue(5);
    ptzSpeedSlider_->setTickPosition(QSlider::TicksBelow);
    ptzSpeedSlider_->setTickInterval(1);
    
    QLabel* lblSpeed = new QLabel("Velocita: 5");
    connect(ptzSpeedSlider_, &QSlider::valueChanged, [lblSpeed](int v){
        lblSpeed->setText(QString("Velocita: %1").arg(v));
    });

    ptzMainLay->addWidget(lblSpeed);
    
    ptzMainLay->addWidget(ptzSpeedSlider_);
    
    chkAutoPtz_ = new QCheckBox("Auto-Tracking (Beta)");
    chkAutoPtz_->setToolTip("Abilita Inseguimento Automatico PTZ");
    chkAutoPtz_->setStyleSheet("font-weight: bold; color: #00FF00;");
    
    ptzMainLay->insertWidget(0, chkAutoPtz_);
    
    lblTrackingStatus_ = new QLabel("Status: Idle");
    lblTrackingStatus_->setStyleSheet("color: yellow; font-size: 10px;");
    ptzMainLay->insertWidget(1, lblTrackingStatus_);

    
    // Auto Watchdog
    autoPtzWatchdog_ = new QTimer(this);
    autoPtzWatchdog_->setInterval(3000); // 3 seconds timeout
    autoPtzWatchdog_->setSingleShot(true);
    connect(autoPtzWatchdog_, &QTimer::timeout, this, &MainWindow::onAutoPtzWatchdog);
    
    connect(chkAutoPtz_, &QCheckBox::stateChanged, [this](int state){
        if (state == Qt::Unchecked) {
            ptzCtrl_->sendPtzCommand("Stop", "Stop", 0);
            autoPtzWatchdog_->stop();
        }
    });



    // Controller Grid
    QGridLayout* grid = new QGridLayout();
    // PTZ Icons (Unicode) and Styling
        // PTZ Icons (Consistent & Circular)
    // Style: Circular buttons, dark grey, hover effect
    QString style = "QPushButton { "
                    "  border-radius: 20px; "  // Radius = Half of min-width (40px)
                    "  min-width: 40px; min-height: 40px; "
                    "  max-width: 40px; max-height: 40px; "
                    "  font-weight: bold; font-size: 16pt; "
                    "  background-color: #353535; color: #E0E0E0; "
                    "  border: 2px solid #202020; "
                    "} "
                    "QPushButton:pressed { background-color: #007ACC; border-color: #005A9E; }";

    // 1. Cardinal (Arrows)
    btnPtzUp_    = new QPushButton(QString::fromUtf8("\xE2\x86\x91")); // Up Arrow (↑)
    btnPtzDown_  = new QPushButton(QString::fromUtf8("\xE2\x86\x93")); // Down Arrow (↓)
    btnPtzLeft_  = new QPushButton(QString::fromUtf8("\xE2\x86\x90")); // Left Arrow (←)
    btnPtzRight_ = new QPushButton(QString::fromUtf8("\xE2\x86\x92")); // Right Arrow (→)
    
    // 2. Diagonals (Arrows)
    btnPtzUpLeft_    = new QPushButton(QString::fromUtf8("\xE2\x86\x96")); // NW Arrow (↖)
    btnPtzUpRight_   = new QPushButton(QString::fromUtf8("\xE2\x86\x97")); // NE Arrow (↗)
    btnPtzDownLeft_  = new QPushButton(QString::fromUtf8("\xE2\x86\x99")); // SW Arrow (↙)
    btnPtzDownRight_ = new QPushButton(QString::fromUtf8("\xE2\x86\x98")); // SE Arrow (↘)

    // Apply Style
    QList<QPushButton*> ptzBtns = {
        btnPtzUp_, btnPtzDown_, btnPtzLeft_, btnPtzRight_, 
        btnPtzUpLeft_, btnPtzUpRight_, btnPtzDownLeft_, btnPtzDownRight_
    };
    for(auto* b : ptzBtns) b->setStyleSheet(style);

    btnZoomIn_ = new QPushButton("Zoom +");
    btnZoomOut_ = new QPushButton("Zoom -");
    btnFocusNear_ = new QPushButton("Focus Near");
    btnFocusFar_ = new QPushButton("Focus Far");
    
    // Grid Layout Logic
    // Row 0
    grid->addWidget(btnPtzUpLeft_, 0, 0);
    grid->addWidget(btnPtzUp_, 0, 1);
    grid->addWidget(btnPtzUpRight_, 0, 2);
    
    // Row 1
    grid->addWidget(btnPtzLeft_, 1, 0);
    grid->addWidget(btnPtzRight_, 1, 2);
    
    // Row 2
    grid->addWidget(btnPtzDownLeft_, 2, 0);
    grid->addWidget(btnPtzDown_, 2, 1);
    grid->addWidget(btnPtzDownRight_, 2, 2);
    
    // Zoom/Focus Columns (Shifted to 3 and 4)
    grid->addWidget(btnZoomIn_, 0, 3);
    grid->addWidget(btnZoomOut_, 1, 3);
    grid->addWidget(btnFocusNear_, 0, 4);
    grid->addWidget(btnFocusFar_, 1, 4);
    
    ptzMainLay->addLayout(grid);
    
    // ADD TO RIGHT LAYOUT
    // right->addWidget(grpPtz_);
    right->addLayout(ptzMainLay);
    right->addStretch();

    // Logic
    std::cout << "DEBUG: MW Init DahuaCameraControl..." << std::endl;
    ptzCtrl_ = new DahuaCameraControl(this);
    std::cout << "DEBUG: MW Init DahuaCameraControl Done" << std::endl;
    ptzStepTimer_ = new QTimer(this);
    ptzStepTimer_->setSingleShot(true);
    connect(ptzStepTimer_, &QTimer::timeout, this, &MainWindow::onPtzStepFinished);

    connect(ptzCtrl_, &DahuaCameraControl::logMessage, [this](const QString& msg){
        // Fixed vibration by removing font-weight changes
        if (msg.startsWith("PTZ Error")) {
             statusLbl_->setStyleSheet("color: red;"); 
        } else {
             statusLbl_->setStyleSheet("color: black;"); 
        }
    statusLbl_->setText(msg);
    });
    auto connectPtz = [&](QPushButton* b, QString code, QString action) {
        connect(b, &QPushButton::pressed, [this, code]() {
            ptzCtrl_->setConnectionDetails(leCamIp_->text(), 80, leCamUser_->text(), leCamPass_->text());
            ptzCtrl_->sendPtzCommand("start", code, ptzSpeedSlider_->value());
        });
        connect(b, &QPushButton::released, [this, code]() {
            ptzCtrl_->sendPtzCommand("stop", code, 5);
        });
    };
    
    connectPtz(btnPtzUp_, "Up", "start");
    connectPtz(btnPtzDown_, "Down", "start");
    connectPtz(btnPtzLeft_, "Left", "start");
    connectPtz(btnPtzRight_, "Right", "start");
    
    // Diagonals (Updated Codes)
    // Diagonals (Continuous Vector Logic) - INVERTED TILT FIX
    // UpLeft: arg1=-Speed (Left), arg2=+Speed (Up) [Fixed]
    connect(btnPtzUpLeft_, &QPushButton::pressed, [this](){
        ptzCtrl_->sendCustomPtz("start", "Continuously", -ptzSpeedSlider_->value(), ptzSpeedSlider_->value(), 0);
    });
    connect(btnPtzUpLeft_, &QPushButton::released, [this](){
        ptzCtrl_->sendCustomPtz("stop", "Continuously", 0, 0, 0);
    });

    // UpRight: arg1=+Speed (Right), arg2=+Speed (Up) [Fixed]
    connect(btnPtzUpRight_, &QPushButton::pressed, [this](){
        ptzCtrl_->sendCustomPtz("start", "Continuously", ptzSpeedSlider_->value(), ptzSpeedSlider_->value(), 0);
    });
    connect(btnPtzUpRight_, &QPushButton::released, [this](){
        ptzCtrl_->sendCustomPtz("stop", "Continuously", 0, 0, 0);
    });

    // DownLeft: arg1=-Speed (Left), arg2=-Speed (Down) [Fixed]
    connect(btnPtzDownLeft_, &QPushButton::pressed, [this](){
        ptzCtrl_->sendCustomPtz("start", "Continuously", -ptzSpeedSlider_->value(), -ptzSpeedSlider_->value(), 0);
    });
    connect(btnPtzDownLeft_, &QPushButton::released, [this](){
        ptzCtrl_->sendCustomPtz("stop", "Continuously", 0, 0, 0);
    });

    // DownRight: arg1=+Speed (Right), arg2=-Speed (Down) [Fixed]
    connect(btnPtzDownRight_, &QPushButton::pressed, [this](){
        ptzCtrl_->sendCustomPtz("start", "Continuously", ptzSpeedSlider_->value(), -ptzSpeedSlider_->value(), 0);
    });
    connect(btnPtzDownRight_, &QPushButton::released, [this](){
        ptzCtrl_->sendCustomPtz("stop", "Continuously", 0, 0, 0);
    });

    connectPtz(btnZoomIn_, "ZoomTele", "start");
    connectPtz(btnZoomOut_, "ZoomWide", "start");
    connectPtz(btnFocusNear_, "FocusNear", "start");
    connectPtz(btnFocusFar_, "FocusFar", "start");



    


    // Connects


    connect(sbDistance_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double d){


        if(worker_) worker_->setManualDistance((float)d);


    });


    connect(sbZoom_, QOverload<int>::of(&QSpinBox::valueChanged), [this](int z){


        if(worker_) worker_->setManualZoom((float)z);


    }); // ADDED


    left->addWidget(cbNoPreview_);
    connect(cbNoPreview_, &QCheckBox::toggled, [this](bool c){ if(worker_) worker_->setEnablePreview(!c); });


    left->addWidget(cbBlur_);


    left->addWidget(cbUseCuda_);





    // Resource Control


    QHBoxLayout* cpuLay = new QHBoxLayout();


    cpuLay->addWidget(new QLabel("Max CPU Threads (0=Auto):"));


    sbCpuThreads_ = new QSpinBox();


    sbCpuThreads_->setRange(0, std::thread::hardware_concurrency());


    sbCpuThreads_->setValue(0); // Default Auto


    sbCpuThreads_->setToolTip("Limita il numero di core CPU usati da OpenCV. 0 = Usa tutti.");


    cpuLay->addWidget(sbCpuThreads_);


    left->addLayout(cpuLay);





    // ================= DETECTION CONFIG =================


    QGroupBox* detGroup = new QGroupBox("Configurazione Rilevamento");


    QVBoxLayout* detLay = new QVBoxLayout(detGroup);





    cbPreset_ = new QComboBox();


    cbPreset_->addItem(QString::fromStdString(presetName(DetectionPreset::Custom)), (int)DetectionPreset::Custom);


    cbPreset_->addItem(QString::fromStdString(presetName(DetectionPreset::Stars)), (int)DetectionPreset::Stars);


    cbPreset_->addItem(QString::fromStdString(presetName(DetectionPreset::Insects)), (int)DetectionPreset::Insects);


    cbPreset_->addItem(QString::fromStdString(presetName(DetectionPreset::Aircraft)), (int)DetectionPreset::Aircraft);


    


    // NEW PRESETS


    cbPreset_->addItem(QString::fromStdString(presetName(DetectionPreset::SmallAnimal)), (int)DetectionPreset::SmallAnimal);


    cbPreset_->addItem(QString::fromStdString(presetName(DetectionPreset::LargeAnimal)), (int)DetectionPreset::LargeAnimal);


    cbPreset_->addItem(QString::fromStdString(presetName(DetectionPreset::Vehicle)), (int)DetectionPreset::Vehicle);


    cbPreset_->addItem(QString::fromStdString(presetName(DetectionPreset::Human)), (int)DetectionPreset::Human);





    // Default: Stars
    cbPreset_->setCurrentIndex(1); // Stars





    detLay->addWidget(new QLabel("Preset:"));


    detLay->addWidget(cbPreset_);





    // MANUAL ZONE


    grpManual_ = new QGroupBox("Parametri Manuali");


    QFormLayout* form = new QFormLayout(grpManual_);





    sbSens_      = new QSpinBox(); sbSens_->setRange(0, 255);


    sbProcEvery_ = new QSpinBox(); sbProcEvery_->setRange(1, 60);


    sbMinHits_   = new QSpinBox(); sbMinHits_->setRange(1, 100);


    sbMinMove_   = new QSpinBox(); sbMinMove_->setRange(0, 200);


    sbAccFrames_ = new QSpinBox(); sbAccFrames_->setRange(1, 200);


    sbAccMinHits_= new QSpinBox(); sbAccMinHits_->setRange(1, 200);





    form->addRow("Sensibilita' (Thresh):", sbSens_);


    form->addRow("Process Every N:", sbProcEvery_);


    form->addRow("Min Hits (Track):", sbMinHits_);


    form->addRow("Min Movement (px):", sbMinMove_);


    form->addRow("Accum Frames (History):", sbAccFrames_);


    form->addRow("Accum Min Hits (Out):", sbAccMinHits_);





    detLay->addWidget(grpManual_);


    left->addWidget(detGroup);





    // Initial sync


    onPresetChanged(cbPreset_->currentIndex());





    connect(cbPreset_, SIGNAL(currentIndexChanged(int)), this, SLOT(onPresetChanged(int)));


    connect(cbEditRoi_, &QCheckBox::toggled,


            this, &MainWindow::onToggleRoiEdit);





    // ================= CONTROL =================


    btnStart_ = new QPushButton("START");


    btnCancel_ = new QPushButton("STOP");


    btnReset_ = new QPushButton("RESET"); // NEW





    connect(btnStart_, &QPushButton::clicked,


            this, &MainWindow::onStart);


    connect(btnCancel_, &QPushButton::clicked,


            this, &MainWindow::onCancel);


    connect(btnReset_, &QPushButton::clicked,


            this, &MainWindow::onReset);





    left->addWidget(btnStart_);


    left->addWidget(btnCancel_);


    left->addWidget(btnReset_);





    // ================= STATUS & TIMER =================


    prog_ = new QProgressBar();


    prog_->setRange(0, 100);


    left->addWidget(prog_);

    // Batch Progress
    progBatch_ = new QProgressBar();
    progBatch_->setRange(0, 100);
    progBatch_->setValue(0);
    progBatch_->setFormat("Batch: %v / %m");
    progBatch_->setStyleSheet("QProgressBar::chunk { background-color: #3add36; }"); // Different color
    left->addWidget(progBatch_);





    QHBoxLayout* statusInfoLay = new QHBoxLayout();


    statusLbl_ = new QLabel("Pronto");


    lblTimer_  = new QLabel("tempo: 00:00:00");
    lblTotalTimer_ = new QLabel("Totale: 00:00:00");
    
    statusInfoLay->addWidget(statusLbl_);
    statusInfoLay->addStretch();
    statusInfoLay->addWidget(lblTimer_);
    statusInfoLay->addWidget(lblTotalTimer_);


    


    left->addLayout(statusInfoLay);


    


    left->addStretch();





    // ================= VIEW =================


    view_ = new QGraphicsView();


    scene_ = new RoiScene(this);


    view_->setScene(scene_);


    view_->setMinimumSize(640, 480);


    mainSplitter->addWidget(view_);
    mainSplitter->setStretchFactor(1, 1); // Give View Priority
    QWidget* rightContainer = new QWidget();
    rightContainer->setLayout(right);
    rightContainer->setMinimumWidth(260);
    mainSplitter->addWidget(rightContainer);
    mainSplitter->setCollapsible(1, false); // View cant collapse





    // ================= WORKER =================
    
    std::cout << "DEBUG: MW Init LiveFrameProcessor..." << std::endl;
    worker_ = new LiveFrameProcessor(this);
    std::cout << "DEBUG: MW Init LiveFrameProcessor Done" << std::endl;
    connect(worker_, &LiveFrameProcessor::primaryObjectDetected, this, &MainWindow::onPrimaryObjectDetected);

    connect(worker_, &LiveFrameProcessor::frameProcessed,
            this, &MainWindow::onLiveFrame);

    // connect(worker_, &LiveFrameProcessor::progress,
            // this, &MainWindow::onProgress);

    connect(worker_, &LiveFrameProcessor::logMessage, [](const QString& msg){ 
        qDebug() << "WORKER LOG (Init):" << msg; 
        std::cout << "WORKER LOG (Init): " << msg.toStdString() << std::endl; 
    });
    connect(worker_, &LiveFrameProcessor::finished, this, &MainWindow::onFinished);

    connect(scene_, &RoiScene::roiPolygonChanged, this, &MainWindow::onRoiPolygonChanged);
    
    std::cout << "DEBUG: MW Init UI Logic..." << std::endl;
    // Disable manual if auto
    connect(chkAutoPtz_, &QCheckBox::stateChanged, this, &MainWindow::onPresetChanged);
    
    resetUiControls();
    // std::cout << "DEBUG: MW Ctor Checkpoint 1" << std::endl;  // Renamed to avoid confusion with real end


    
    // Connect Timer


    connect(worker_, &LiveFrameProcessor::elapsedTimeUpdated, this, &MainWindow::onElapsedTime);





    // Check CUDA


    if (worker_ && worker_->isCudaAvailable()) {


        cbUseCuda_->setEnabled(true);


        cbUseCuda_->setText("Usa Accelerazione GPU (CUDA) - Rilevata!");


    } else {


        cbUseCuda_->setEnabled(false);


        cbUseCuda_->setToolTip("Nessuna GPU NVIDIA compatibile rilevata o supporto CUDA non compilato.");


    }
    std::cout << "DEBUG: MW Ctor End" << std::endl;
}





MainWindow::~MainWindow() = default;











// ======================================================


// INPUT MULTI FILE


// ======================================================





void MainWindow::onSourceChanged()
{
    // If we have radio buttons, we check which one is active
    bool isNet = rbSourceNet_ && rbSourceNet_->isChecked();
    bool isFile = rbSourceFile_ && rbSourceFile_->isChecked();

    // Show/Hide relevant controls
    if (leRtspUrl_) leRtspUrl_->setVisible(isNet);
    if (btnConnectRtsp_) btnConnectRtsp_->setVisible(isNet);
    if (inEdit_) inEdit_->setVisible(isFile);
    
    // Find button by name since it's local
    QPushButton* btn = findChild<QPushButton*>("btnBrowseInput");
    if (btn) btn->setVisible(isFile);

    // Update labels or hints if needed
    if (isNet) {
    statusLbl_->setText(isNet ? "Analisi Live Stream" : QString("Analisi %1 / %2").arg(currentIndex_ + 1).arg(inputFiles_.size()));
    }
    
    // Refresh names based on current mode
    updateAutoOutputNames();
}

// Safe Browse Input
void MainWindow::onBrowseInput()
{
    try {
        std::cout << "TRACE: Opening File Dialog..." << std::endl;
        QStringList files = QFileDialog::getOpenFileNames(this, "Seleziona Video", "", "Video (*.mp4 *.avi *.mkv *.mov);;Tutti (*.*)", nullptr, QFileDialog::DontUseNativeDialog);
        std::cout << "TRACE: File Dialog Closed. Files: " << files.size() << std::endl;
        if (files.isEmpty()) return;

        inputFiles_ = files;
        currentIndex_ = 0;
        
        lblFileInfo_->setText(QString("Selezionati %1 file").arg(files.size()));
        
        // Auto-set output folder to same as first input if empty
        if (!files.isEmpty() && outDirEdit_->text().isEmpty()) {
             QFileInfo fi(files.first());
             outDirEdit_->setText(fi.absolutePath());
        }

        updateAutoOutputNames();
        
        // Try to load first frame of first video
        onLoadFirstFrame();
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Errore Input", e.what());
    } catch (...) {
        QMessageBox::critical(this, "Errore", "Errore sconosciuto selezione file.");
    }
}





// Safe Browse Output
void MainWindow::onBrowseOutputDir()
{
    try {
        std::cout << "TRACE: Opening Output Dir Dialog..." << std::endl;
        QString d = QFileDialog::getExistingDirectory(this, "Cartella output", "", QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
        std::cout << "TRACE: Output Dir Dialog Closed" << std::endl;
        if (d.isEmpty()) return;
        outDirEdit_->setText(d);
        updateAutoOutputNames();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Errore", QString("Eccezione in Browse Output: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Errore", "Errore sconosciuto in Browse Output");
    }
}


void MainWindow::updateAutoOutputNames()
{
    if (outDirEdit_->text().isEmpty()) return;

    QString base;
    if (rbSourceNet_->isChecked()) {
        base = "LiveStream_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmm");
    } else {
        if (inputFiles_.isEmpty()) return;
        QFileInfo info(inputFiles_.at(currentIndex_));
        base = info.completeBaseName();
    }
    
    QDir outDir(outDirEdit_->text());
    // Create specific subfolder for this video to avoid mess
    QString subFolderName = base;
    // Sanitize folder name
    subFolderName.replace(":", "").replace("/", "").replace("\\", ""); 
    
    QDir subDir(outDir.filePath(subFolderName));
    if (!subDir.exists()) {
        // We don't create it here to avoid empty folders on browse. 
        // Logic in onFinished/startCurrentFile handles creation.
    }

    QString vidName = base + "_overlay.mp4";
    QString csvName = base + ".csv";
    
    // We construct the full path thinking about the subfolder
    QString fullVidPath = subDir.filePath(vidName);
    QString fullCsvPath = subDir.filePath(csvName);

    outVideoEdit_->setText(fullVidPath);
    outCsvEdit_->setText(fullCsvPath);
}





// ... (in startCurrentFile, we must create this dir)





void MainWindow::startCurrentFile()

{
    qDebug() << "DEBUG: startCurrentFile called";
    std::cout << "DEBUG: startCurrentFile called" << std::endl;
    

    if (!worker_) {
        std::cout << "CRITICAL: Worker invalid at start of startCurrentFile" << std::endl;
        return;
    }

    // --- FIX: Force Stop Previous Playback --- 
    std::cout << "DEBUG: Ensuring previous playback is stopped..." << std::endl;
    std::cout << "TRACE: MW Calling stopPlayback... (Signals Blocked)" << std::endl;
    bool oldState = worker_->blockSignals(true);
    worker_->stopPlayback();
    worker_->blockSignals(oldState);
    std::cout << "TRACE: MW stopPlayback RETURNED. (Signals Restored)" << std::endl;
    std::cout << "DEBUG: Previous playback stopped." << std::endl;


    QString file;


    bool isNet = rbSourceNet_->isChecked();


    


    if (isNet) {


        file = leRtspUrl_->text();


        if (file.isEmpty()) {


             QMessageBox::warning(this, "Errore", "Inserisci URL RTSP valido");


             return;


        }


    } else {
        if (currentIndex_ >= 0 && currentIndex_ < inputFiles_.size()) {
            file = inputFiles_[currentIndex_];
        }
    }





    statusLbl_->setText(isNet ? "Analisi Live Stream" : QString("Analisi %1 / %2").arg(currentIndex_ + 1).arg(inputFiles_.size()));





    prog_->setValue(0);





    // Setup Output Names


    updateAutoOutputNames(); // Refresh timestamp for live


    


    // Create Subdir Logic


    QString baseName; 


    if (isNet) {


        baseName = "Live_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmm");


    } else {


        baseName = QFileInfo(file).completeBaseName();


    }


    


    QDir outDir(outDirEdit_->text());


    if (!outDir.exists(baseName)) {


        outDir.mkdir(baseName);


    }


    


    QString subDirPath = outDir.filePath(baseName);


    


    // Determine specific filenames inside the subdir


    QDir subDir(subDirPath);


    QString outVideo = subDir.filePath(baseName + "_overlay.mp4");


    QString outCsv = subDir.filePath(baseName + ".csv");


    


    outVideoEdit_->setText(outVideo);


    outCsvEdit_->setText(outCsv); // Display Update


    


    // FIX: Set working dir explicitly so rotation knows where to create new folders
    worker_->setWorkingDir(subDirPath);
    std::cout << "TRACE: MW Calling openVideo..." << std::endl;
    worker_->setEnablePreview(!cbNoPreview_->isChecked());
    worker_->openVideo(file);
    std::cout << "TRACE: MW openVideo RETURNED." << std::endl;


    worker_->setOutputVideoPath(outVideo);


    worker_->setOutputCsvPath(outCsv);





    worker_->setUseRoi(cbUseRoi_->isChecked());


    worker_->setDrawRoi(cbDrawRoi_->isChecked());


    


    worker_->setEnablePreview(!cbNoPreview_->isChecked());
    worker_->setUseCuda(cbUseCuda_->isChecked());
    
    // Connect File Info Signal
    connect(worker_, &LiveFrameProcessor::videoInfoReady, this, &MainWindow::onVideoInfoReady);



    worker_->setUseCuda(cbUseCuda_->isChecked());


    worker_->setMaxCpuThreads(sbCpuThreads_->value());


    worker_->setClipOnlyMode(cbClipOnly_->isChecked());


    worker_->setSaveSnapshots(cbSaveSnapshots_->isChecked()); 


    


    worker_->params = paramsFromUi();


    


    if (scene_) onRoiPolygonChanged(scene_->roiPolygon()); 


    if (scene_) onRoiPolygonChanged(scene_->roiPolygon()); 


    if (!worker_) {
         qCritical() << "CRITICAL: Worker is NULL in startCurrentFile";
         std::cout << "CRITICAL: Worker is NULL in startCurrentFile" << std::endl;
         QMessageBox::critical(this, "Errore", "Errore interno: Worker non inizializzato.");
         return;
    }

    try {
        qDebug() << "DEBUG: Invoking worker->startPlayback()";
        std::cout << "DEBUG: Invoking worker->startPlayback()" << std::endl;
        std::cout << "TRACE: MW Calling startPlayback..." << std::endl;
    // LOGIC: Handle Preview vs Analysis Start
    // FIX: Force reset of skipAnalysis based on current mode
    worker_->setSkipAnalysis(startingPreview_); 
    
    if (startingPreview_) {
        // Redundant set but harmless, keeps existing UI logic inside block
    statusLbl_->setText(isNet ? "Analisi Live Stream" : QString("Analisi %1 / %2").arg(currentIndex_ + 1).arg(inputFiles_.size()));
             worker_->params = paramsFromUi();
    statusLbl_->setText(isNet ? "Analisi Live Stream" : QString("Analisi %1 / %2").arg(currentIndex_ + 1).arg(inputFiles_.size()));
    }
    
        worker_->startPlayback();
    std::cout << "TRACE: MW startPlayback RETURNED." << std::endl;
    } catch (const std::exception& e) {
        qCritical() << "CRITICAL EXCEPTION in startPlayback:" << e.what();
        std::cout << "CRITICAL EXCEPTION in startPlayback:" << e.what() << std::endl;
        QMessageBox::critical(this, "Errore Critico", QString("Eccezione: %1").arg(e.what()));
    } catch (...) {
        qCritical() << "CRITICAL UNKNOWN EXCEPTION in startPlayback";
        std::cout << "CRITICAL UNKNOWN EXCEPTION in startPlayback" << std::endl;
        QMessageBox::critical(this, "Errore Critico", "Eccezione sconosciuta durante startPlayback");
    }


}





bool MainWindow::hasMoreFiles() const


{


    return (currentIndex_ + 1) < inputFiles_.size();


}













// ======================================================


// ROI EDIT / STOP


// ======================================================





void MainWindow::onToggleRoiEdit(bool on)


{


    if (scene_)


        scene_->setEditMode(on);


}





void MainWindow::onCancel()
{
    // SMART STOP: 
    // If Net & Analysis is Running -> Stop Analysis Only (Keep Preview)
    // If Net & Preview -> Fully Stop
    // If File -> Fully Stop

    if (!worker_) return;

    bool isNet = rbSourceNet_ && rbSourceNet_->isChecked();
    bool isAnalyzing = !worker_->isSkippingAnalysis();
    
    // Check if we are in "Hybrid State" (Running + Analysis On + RTSP)
    if (isNet && isAnalyzing) {
         std::cout << "SMART STOP: Stopping Analysis, keeping Preview." << std::endl;
         worker_->setSkipAnalysis(true); // Go back to "Preview Mode"
         statusLbl_->setText("Preview (Analisi Fermata)");
         btnStart_->setEnabled(true); // Allow re-start
         // btnCancel_ remains enabled to allow full stop
         return;
    }

    // Default Full Stop
    std::cout << "TRACE: MW Calling stopPlayback... (Full Stop)" << std::endl;
    bool oldState = worker_->blockSignals(true);
    worker_->stopPlayback();
    worker_->blockSignals(oldState);
    std::cout << "TRACE: MW stopPlayback RETURNED." << std::endl;
    
    statusLbl_->setText("Arrestato");
    btnStart_->setEnabled(true);
    btnCancel_->setEnabled(false);
    prog_->setValue(0);
}

void MainWindow::onReset()


{


    // Stop pending work


    if (worker_) {


        std::cout << "TRACE: MW Calling stopPlayback... (Signals Blocked)" << std::endl;
    bool oldState = worker_->blockSignals(true);
    worker_->stopPlayback();
    worker_->blockSignals(oldState);
    std::cout << "TRACE: MW stopPlayback RETURNED. (Signals Restored)" << std::endl;


        delete worker_;


        worker_ = nullptr;


    }


    


    // Clear Files & Data


    inputFiles_.clear();


    currentIndex_ = 0;


    


    // Clear UI inputs


    if(inEdit_) inEdit_->clear();


    // if(outDirEdit_) outDirEdit_->clear(); // Keep output dir? user said "resetta tutto". okay.


    if(outDirEdit_) outDirEdit_->clear();


    if(outVideoEdit_) outVideoEdit_->clear();


    if(outCsvEdit_) outCsvEdit_->clear();


    if(leRtspUrl_) leRtspUrl_->clear();


    


    // Reset Status


    if(prog_) prog_->setValue(0);


    if(statusLbl_) statusLbl_->setText("Pronto (Reset Completo)");


    if(lblFileInfo_) lblFileInfo_->clear();


    resetUiControls();


    


    batchTimer_.invalidate();


    


    // Clear ROI and Scene Background


    if(scene_) {


        scene_->clearRoi();


        scene_->setBackground(QImage()); 


        scene_->update();


    }


    


    // RECREATE WORKER
    if (worker_) {
        std::cout << "TRACE: MW Calling stopPlayback... (Signals Blocked)" << std::endl;
    bool oldState = worker_->blockSignals(true);
    worker_->stopPlayback();
    worker_->blockSignals(oldState);
    std::cout << "TRACE: MW stopPlayback RETURNED. (Signals Restored)" << std::endl; // Ensure threads stop
        worker_->deleteLater(); // Clean up old instance
    }
    worker_ = new LiveFrameProcessor(this);
    connect(worker_, &LiveFrameProcessor::primaryObjectDetected, this, &MainWindow::onPrimaryObjectDetected);

    connect(worker_, &LiveFrameProcessor::frameProcessed, this, &MainWindow::onLiveFrame, Qt::QueuedConnection);
    // connect(worker_, &LiveFrameProcessor::progress, this, &MainWindow::onProgress);
    connect(worker_, &LiveFrameProcessor::logMessage, [](const QString& msg){ 
        qDebug() << "WORKER LOG:" << msg; 
        std::cout << "WORKER LOG: " << msg.toStdString() << std::endl; 
    });
    connect(worker_, &LiveFrameProcessor::error, this, &MainWindow::onWorkerError);
    connect(worker_, &LiveFrameProcessor::finished, this, &MainWindow::onFinished);
    connect(worker_, &LiveFrameProcessor::elapsedTimeUpdated, this, &MainWindow::onElapsedTime);

    


    // Re-check CUDA capability on new worker


    if (worker_ && worker_->isCudaAvailable()) {


        cbUseCuda_->setEnabled(true);


        cbUseCuda_->setText("Usa Accelerazione GPU (CUDA) - Rilevata!");


    } else {


        cbUseCuda_->setEnabled(false);


        cbUseCuda_->setToolTip("Nessuna GPU NVIDIA compatibile rilevata.");


        cbUseCuda_->setChecked(false);


    }


}





void MainWindow::onPresetChanged(int index)


{


    DetectionPreset p = (DetectionPreset)cbPreset_->itemData(index).toInt();


    


    bool isCustom = (p == DetectionPreset::Custom);


    grpManual_->setEnabled(isCustom); 





    DetectionParams dp = presetToParams(p);


    uiFromParams(dp);


    


    if (cbBlur_) cbBlur_->setChecked(dp.noiseReduction);


}





void MainWindow::uiFromParams(const DetectionParams& p)


{


    if (!sbSens_) return;


    sbSens_->setValue(p.sensitivity);


    sbProcEvery_->setValue(p.processEveryN);


    sbMinHits_->setValue(p.minHits);


    sbMinMove_->setValue(p.minMovementPx);


    sbAccFrames_->setValue(p.accumFrames);


    sbAccMinHits_->setValue(p.accumMinHits);


}





DetectionParams MainWindow::paramsFromUi() const


{


    DetectionParams p;


    if (sbSens_) p.sensitivity   = sbSens_->value();


    if (sbProcEvery_) p.processEveryN = sbProcEvery_->value();


    if (sbMinHits_) p.minHits       = sbMinHits_->value();


    if (sbMinMove_) p.minMovementPx = sbMinMove_->value();


    if (sbAccFrames_) p.accumFrames   = sbAccFrames_->value();


    if (sbAccMinHits_) p.accumMinHits  = sbAccMinHits_->value();


    


    if (cbBlur_) p.noiseReduction = cbBlur_->isChecked(); 


    


    p.blobMinArea = 2; 


    p.blobMaxArea = 100; 


    


    DetectionPreset active = (DetectionPreset)cbPreset_->currentData().toInt();


    if (active != DetectionPreset::Custom) {


        return presetToParams(active);


    }


    


    return p;


}





// ======================================================


// MENU & HELP


// ======================================================





void MainWindow::createMenu()


{


    QMenuBar* bar = menuBar();


    QMenu* menuHelp = bar->addMenu("Aiuto");





    QAction* actHelp = new QAction("Guida", this);


    connect(actHelp, &QAction::triggered, this, &MainWindow::onActionHelp);


    menuHelp->addAction(actHelp);





    QAction* actInfo = new QAction("Info / Crediti", this);


    connect(actInfo, &QAction::triggered, this, &MainWindow::onActionInfo);


    menuHelp->addAction(actInfo);


}


void MainWindow::onActionHelp()
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("Guida - Analisi Video Sperimentale Avanzata 4.5 Beta");
    dlg->resize(600, 500);

    QVBoxLayout* lay = new QVBoxLayout(dlg);
    QTextBrowser* txt = new QTextBrowser();
    
    // Simple dark style for the dialog content
    txt->setStyleSheet("background-color: #1e1e1e; color: #dcdcdc; font-family: 'Segoe UI', sans-serif;");

    QString html = R"(
    <h2>Analisi Video Sperimentale Avanzata 4.5 Beta</h2>
    <p>Questo software &egrave; progettato per l'analisi e il rilevamento di oggetti in movimento in file video.</p>
    
    <h3>Funzioni Principali (Novit&agrave;):</h3>
    <ul>
        <li><b>Input Multiplo e Batch</b>: Analisi sequenziale di pi&ugrave; file con barra di avanzamento generale.</li>
        <li><b>Snapshots Zoom</b>: Salva automaticamente sia la panoramica che uno zoom centrato sull'oggetto.</li>
        <li><b>Info File e Timer</b>: Visualizzazione dettagliata di Risoluzione, FPS e Timer cumulativo.</li>
    </ul>

    <h3>Output:</h3>
    <p>Il programma genera, per ogni video:</p>
    <ul>
        <li>Un file <b>CSV</b> con i dati di tracciamento.</li>
        <li>Un video <b>MP4</b> ("_overlay") con evidenziati gli oggetti.</li>
        <li>Cartella <b>Screenshots</b> con foto "panoramiche" e "zoom".</li>
    </ul>

    <hr>
    <h3>Coming Soon (Versione Beta 4.5_7):</h3>
    <ul>
        <li>Integrazione comandi PTZ (Pan-Tilt-Zoom).</li>
        <li>Studio per il Tracking Automatico (Inseguimento).</li>
    </ul>

    <div style="background-color: #333; padding: 10px; margin-top: 10px; border-left: 5px solid red;">
        <b>DISCLAIMER LEGALE E TECNICO:</b><br>
        <small>L'autore non si assume nessuna responsabilit&agrave; sull'utilizzo del software per scopi diversi da quelli indicati in questa descrizione (ricerca amatoriale e sperimentale). Le stime fornite sono approssimative e non certificate. L'uso improprio dei dati generati &egrave; a totale rischio dell'utente.</small>
    </div>
    )";

    txt->setHtml(html);
    lay->addWidget(txt);

    QDialogButtonBox* bbox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(bbox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    lay->addWidget(bbox);

    dlg->exec();
}

// Credits with links
void MainWindow::onActionInfo()
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("Info / Crediti");
    dlg->resize(500, 450);
    QVBoxLayout* lay = new QVBoxLayout(dlg);
    
    QTextBrowser* tb = new QTextBrowser();
    tb->setOpenExternalLinks(true); // Enable clickable links
    
    QString html = R"(
    <style>
        body { font-family: Segoe UI, sans-serif; text-align: center; }
        h2 { color: #2c3e50; }
        h3 { color: #E67E22; margin-bottom: 5px; }
        a { color: #2980b9; text-decoration: none; font-weight: bold; }
        .sub { color: #7f8c8d; font-size: 0.9em; }
    </style>
    
    <h2>Analisi Video Sperimentale Avanzata</h2>
    <div class="sub">Versione 4.5 Clean Beta - 2025</div>
    
    <h3>Ideazione e Ricerca</h3>
    <p><b>Giuseppe Aloe (Nick: Pino)</b><br>
    Libero ricercatore in campo ufologico – Freelance<br>
    Italia, Calabria – Corigliano Rossano<br>
    Email: pino88700@gmail.com</p>

    <h3>Canali Social</h3>
    <p>
    Canale YouTube:<br>
    <a href="https://www.youtube.com/channel/UCYA1aW2sMw5IZaxEP2TUNMw">Italia Alieni e Dintorni</a>
    </p>
    <p>
    Pagina Facebook:<br>
    <a href="https://www.facebook.com/groups/708448406416941">Alieni e Dintorni</a>
    </p>
    
    <hr>
    <h3>Sviluppo Tecnico</h3>
    <p>Coding & AI Engineering by<br><b>Gemini (Google DeepMind)</b><br>
    <span class="sub">Implementazione C++, CUDA Optimization, Qt GUI</span></p>
    )";
    
    tb->setHtml(html);
    lay->addWidget(tb);
    
    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bb, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    lay->addWidget(bb);
    
    dlg->exec();
}





void MainWindow::onFinished()


{


    if (worker_) std::cout << "TRACE: MW Calling stopPlayback... (Signals Blocked)" << std::endl;
    bool oldState = worker_->blockSignals(true);
    worker_->stopPlayback();
    worker_->blockSignals(oldState);
    std::cout << "TRACE: MW stopPlayback RETURNED. (Signals Restored)" << std::endl;

    // Accumulate Timer
    totalBatchTimeMs_ += lastReportedMs_;
    lastReportedMs_ = 0;


    


    // Only move files in File Mode


    if (rbSourceFile_->isChecked() && currentIndex_ >= 0 && currentIndex_ < inputFiles_.size()) {


        QString currentSource = inputFiles_.at(currentIndex_);


        QFileInfo srcInfo(currentSource);


        QString baseName = srcInfo.completeBaseName();


        QString fileName = srcInfo.fileName();


        


        QDir outDir(outDirEdit_->text());


        if (outDir.exists(baseName)) {


            QDir subDir(outDir.filePath(baseName));


            QString destPath = subDir.filePath(fileName);


            


            if (QFile::exists(destPath)) {


                QFile::remove(destPath);


            }


            


            if (QFile::copy(currentSource, destPath)) { 
                QFileInfo d(destPath); 
                if (d.exists() && d.size() > 0) { 
                    QFile::remove(currentSource); 
                } 
            } else { 
                QFile::rename(currentSource, destPath); 
            }
        }


    }





    if (hasMoreFiles() && rbSourceFile_->isChecked())


    {


        QTimer::singleShot(100, this, &MainWindow::advanceToNextFile);


        return;


    }





    statusLbl_->setText("Batch completato / Stop Live");


    


    // Garbage removed: worker_->openVideo(file, outDirEdit_->text());


}


void MainWindow::onLoadFirstFrame()


{


    qDebug() << "onLoadFirstFrame: STARTED";
    
    QString path;
    if (rbSourceNet_ && rbSourceNet_->isChecked()) {
        path = leRtspUrl_->text();
    } else {
        if (inputFiles_.isEmpty()) return;
        path = inputFiles_.first();
    }

    if(path.isEmpty()) return;

    cv::VideoCapture cap(path.toStdString());


    if (!cap.isOpened()) { qDebug() << "Failed to open " << path; return; }


    cv::Mat frame;


    cap >> frame;


    if (frame.empty()) return;


    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);


    QImage img((const uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);


    img = img.copy();


        QList<QGraphicsView*> views = findChildren<QGraphicsView*>();


    if (!scene_) {


         scene_ = new RoiScene(this);


         if (!views.isEmpty()) views.first()->setScene(scene_);


    }


    if(scene_) scene_->clearRoi(); // Replaced clear() to prevent dangling pointers


    scene_->setBackground(img);


    scene_->update();


    if (view_) view_->fitInView(scene_->itemsBoundingRect(), Qt::KeepAspectRatio);


    qDebug() << "onLoadFirstFrame: SHOWED IMAGE";


}





void MainWindow::onLiveFrame(const QImage& img)
{
    static int trc = 0; if(trc++ < 10) std::cout << "TRACE: onLiveFrame (GUI update)" << std::endl;
    try {
        if (!img.isNull() && scene_) {
            scene_->setImage(img);
            if (view_) view_->fitInView(scene_->itemsBoundingRect(), Qt::KeepAspectRatio);
        }
    } catch (const std::exception& e) {
        std::cout << "ERROR: Exception in onLiveFrame: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "ERROR: Unknown exception in onLiveFrame" << std::endl;
    }
}

void MainWindow::onProgress(int percent)
{
    if (prog_) prog_->setValue(percent);
}

void MainWindow::onWorkerError(const QString& msg)
{
    QMessageBox::critical(this, "Errore", msg);
    if (statusLbl_) statusLbl_->setText("Errore: " + msg);
}

void MainWindow::onElapsedTime(const QString& timeStr, qint64 ms)
{
    if (lblTimer_) lblTimer_->setText("Tempo: " + timeStr);
    
    lastReportedMs_ = ms;
    
    // Update total
    qint64 total = totalBatchTimeMs_ + ms;
    int seconds = (int)(total / 1000);
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    QString totalStr = QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
        
    if (lblTotalTimer_) lblTotalTimer_->setText("Totale: " + totalStr);
}

void MainWindow::onStart()
{
    if (!worker_) return;
    
    if (rbSourceFile_->isChecked() && inputFiles_.isEmpty()) return;
    if (rbSourceNet_->isChecked() && leRtspUrl_->text().isEmpty()) return;

    // Reset Totals
    totalBatchTimeMs_ = 0;
    lastReportedMs_ = 0;
    if(lblTotalTimer_) lblTotalTimer_->setText("Totale: 00:00:00");

    batchTimer_.start(); 

    if (rbSourceFile_->isChecked()) {
        currentIndex_ = qMax(0, currentIndex_);
        if (progBatch_) {
            progBatch_->setRange(0, inputFiles_.size());
            progBatch_->setValue(currentIndex_);
            progBatch_->setFormat(QString("Batch: %1 / %2").arg(currentIndex_ + 1).arg(inputFiles_.size()));
        }
    }
    startCurrentFile();
}

void MainWindow::onPrimaryObjectDetected(QRectF rect, QPointF center, QSize frameSize)
{
    // Placeholder
}

void MainWindow::onPtzStepFinished()
{
    if (ptzCtrl_) ptzCtrl_->stop();
    if (ptzStepTimer_) ptzStepTimer_->stop();
}

void MainWindow::resetUiControls()
{
    if (prog_) prog_->setValue(0);
    if (statusLbl_) statusLbl_->setText("Pronto");
}

void MainWindow::onRoiPolygonChanged(const QPolygonF& poly)
{
    if (!scene_) return;
    
    // Logic to update processing ROI if needed
    // Currently just logs or updates UI state
    // std::cout << "DEBUG: ROI Polygon Changed" << std::endl;
}

void MainWindow::onAutoPtzWatchdog()
{
    // Placeholder for PTZ watchdog logic
    // Could reset position if no motion detected for X seconds
}

void MainWindow::onVideoInfoReady(int width, int height, double fps, int totalFrames, QString codec)
{
    if (lblFileInfo_) {
        lblFileInfo_->setText(QString("Res: %1x%2 | FPS: %3 | Frames: %4 | Codec: %5")
            .arg(width).arg(height).arg(fps, 0, 'f', 2).arg(totalFrames).arg(codec));
    }
}



void MainWindow::onConnectRtsp() {
    QString url = leRtspUrl_->text();
    if (url.isEmpty()) { QMessageBox::warning(this, "Errore", "URL vuoto"); return; }
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    cv::VideoCapture cap;
    bool ok = cap.open(url.toStdString());
    cap.release(); // release immediately
    QApplication::restoreOverrideCursor();
    
    if (ok) {
        // Connection Successful
        // Automatically switch to Net Source and Start
        rbSourceNet_->setChecked(true);
    statusLbl_->setText("Connessione OK. Avvio Preview...");
        // User said "telecamera deve essere in preview". I will check it to false just in case.
        if (cbNoPreview_->isChecked()) {
             cbNoPreview_->setChecked(false);
        }

        startingPreview_ = true; // FORCE PREVIEW MODE
        onStart();
        startingPreview_ = false; // RESET FOR NORMAL START
        
    } else {
        QMessageBox::critical(this, "Errore", "Impossibile connettersi a RTSP.\nVerifica IP, Porta, User/Pass.");
    }
}

void MainWindow::advanceToNextFile()
{
    currentIndex_++;
    if (currentIndex_ >= inputFiles_.size()) {
        onFinished();
        return;
    }
    
    // Update Batch Progress
    if(progBatch_) {
         progBatch_->setValue(currentIndex_);
         progBatch_->setFormat(QString("Batch: %1 / %2").arg(currentIndex_ + 1).arg(inputFiles_.size()));
    }
    
    startCurrentFile();
}