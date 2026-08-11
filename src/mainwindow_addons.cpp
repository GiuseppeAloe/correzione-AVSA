
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

void MainWindow::onActionInfo()
{
    QString credits = R"(
    <h3>Ideazione e Ricerca</h3>
    <p><b>Giuseppe Aloe (Nick: Pino)</b><br>
    Libero ricercatore in campo ufologico – Freelance<br>
    Italia, Calabria – Corigliano Rossano<br>
    Email: pino88700@gmail.com</p>

    <p><b>Canale YouTube:</b><br>
    Italia Alieni e Dintorni<br>
    <a href="https://www.youtube.com/channel/UCYA1aW2sMw5IZaxEP2TUNMw">https://www.youtube.com/channel/UCYA1aW2sMw5IZaxEP2TUNMw</a></p>

    <p><b>Pagina Facebook:</b><br>
    Alieni e Dintorni<br>
    facebook.com/groups/708448406416941</p>
    
    <hr>
    <p><i>Implementazione Tecnica & Integrazione CUDA (In Progress)</i></p>
    )";

    QMessageBox::about(this, "Info / Crediti", credits);
}
