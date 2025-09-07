#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QCoreApplication>
#include <algorithm>

static double toMB(qint64 bytes) {
    return bytes / (1024.0 * 1024.0);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    auto *root = new QVBoxLayout(ui->centralwidget);
    root->setContentsMargins(8,8,8,8);
    root->setSpacing(8);

    tabs = new QTabWidget(this);
    root->addWidget(tabs, 1);

    buildTabs();
    startServer();

    resize(1200, 800);
}

MainWindow::~MainWindow() { delete ui; }

// ================== Server ==================
void MainWindow::startServer() {
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
    if (!server->listen(QHostAddress::LocalHost, serverPort)) {
        server->listen(QHostAddress::LocalHost, 0); // si 5555 ocupado, usa cualquiera
    }
    setWindowTitle(QString("GUI-PROFILE (server port %1)").arg(server->serverPort()));
}

void MainWindow::onNewConnection() {
    if (client) {
        auto s = server->nextPendingConnection();
        s->close();
        s->deleteLater();
        return;
    }
    client = server->nextPendingConnection();
    rxBuffer.clear();
    connect(client, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &MainWindow::onClientDisconnected);

    // limpia estado
    alive.clear(); aliveBytesByFile.clear(); aliveCountByFile.clear();
    liveBytes = 0; totalAllocs = 0; maxLiveMB = 0;
    rowOfPtr.clear();
    tblMap->setRowCount(0);
    ovSeries->clear();
    t0_ms = -1;
    updateOverview();
    updateTop3();
    updateByFileChart();
}

void MainWindow::onReadyRead() {
    rxBuffer += client->readAll();
    int idx;
    while ((idx = rxBuffer.indexOf('\n')) >= 0) {
        QByteArray line = rxBuffer.left(idx);
        rxBuffer.remove(0, idx+1);
        processLine(line.trimmed());
    }
}

void MainWindow::processLine(const QByteArray& line) {
    QJsonParseError err{};
    auto doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;
    auto o = doc.object();
    const QString type = o.value("type").toString();
    if (type == "alloc") handleAlloc(o);
    else if (type == "free") handleFree(o);
    else if (type == "tick") handleTick(o);
}

void MainWindow::handleAlloc(const QJsonObject& o) {
    Alloc a;
    a.ptr   = static_cast<quintptr>(o.value("ptr").toVariant().toULongLong());
    a.bytes = static_cast<qint64>(o.value("bytes").toVariant().toLongLong());
    a.type  = o.value("array").toBool() ? "new[]" : "new";
    a.file  = o.value("file").toString();
    a.line  = o.value("line").toInt();
    a.ts_ms = static_cast<qint64>(o.value("ts_ms").toVariant().toLongLong());

    if (alive.contains(a.ptr)) return; // duplicado
    alive.insert(a.ptr, a);
    liveBytes += a.bytes;
    totalAllocs += 1;

    aliveCountByFile[a.file] += 1;
    aliveBytesByFile[a.file] += a.bytes;

    updateMapAdd(a);
    updateOverview();
    updateTop3();
    updateByFileChart();
}

void MainWindow::handleFree(const QJsonObject& o) {
    const quintptr ptr = static_cast<quintptr>(o.value("ptr").toVariant().toULongLong());
    auto it = alive.find(ptr);
    if (it == alive.end()) return;
    const Alloc a = it.value();
    alive.erase(it);

    liveBytes -= a.bytes;
    if (liveBytes < 0) liveBytes = 0;

    aliveCountByFile[a.file] -= 1;
    aliveBytesByFile[a.file] -= a.bytes;
    if (aliveCountByFile[a.file] <= 0) aliveCountByFile.remove(a.file);
    if (aliveBytesByFile[a.file] <= 0) aliveBytesByFile.remove(a.file);

    updateMapRemove(ptr);
    updateOverview();
    updateTop3();
    updateByFileChart();
}

void MainWindow::handleTick(const QJsonObject& o) {
    const double t_ms = o.value("t_ms").toDouble();
    if (t0_ms < 0) t0_ms = t_ms;
    const double t = (t_ms - t0_ms) / 1000.0;
    const double mb = toMB(liveBytes);
    ovSeries->append(t, mb);

    if (mb > maxLiveMB) maxLiveMB = mb;
    ovAxisX->setRange(std::max(0.0, t - 60.0), t); // ventana 60s
    if (mb > ovAxisY->max()) ovAxisY->setRange(0, mb * 1.2);

    updateOverview();
}

void MainWindow::onClientDisconnected() {
    onClientDisconnectedComputeLeaks();
    client->deleteLater();
    client = nullptr;
}

void MainWindow::onClientDisconnectedComputeLeaks() {
    const qint64 leaksBytes = liveBytes;
    qint64 maxLeak = 0;
    QString maxLeakFile;
    QHash<QString,qint64> leaksByFile;
    QHash<QString,qint64> leaksCountByFile;
    QLineSeries* leakTimesTmp = new QLineSeries();

    for (const auto& a : alive) {
        leaksByFile[a.file] += a.bytes;
        leaksCountByFile[a.file] += 1;
        if (a.bytes > maxLeak) { maxLeak = a.bytes; maxLeakFile = a.file; }
        if (t0_ms >= 0) {
            double tt = (a.ts_ms - t0_ms) / 1000.0;
            leakTimesTmp->append(tt, toMB(a.bytes));
        }
    }

    QString fileMost; qint64 countMost = 0;
    for (auto it = leaksCountByFile.begin(); it != leaksCountByFile.end(); ++it) {
        if (it.value() > countMost) { countMost = it.value(); fileMost = it.key(); }
    }

    const double leakRate = (totalAllocs > 0) ? (double)alive.size() / (double)totalAllocs : 0.0;

    lblLeaksTotalMB->setText(QString("Total fuga: %1 MB").arg(QString::number(toMB(leaksBytes), 'f', 2)));
    lblLeakLargest->setText(QString("Leak más grande: %1 MB (%2)")
                                .arg(QString::number(toMB(maxLeak),'f',2)).arg(maxLeakFile.isEmpty()?"N/A":maxLeakFile));
    lblLeakFileMost->setText(QString("Archivo con más leaks: %1 (%2)").arg(fileMost.isEmpty()?"N/A":fileMost).arg(countMost));
    lblLeakRate->setText(QString("Tasa de leaks: %1%").arg(QString::number(leakRate*100.0, 'f', 2)));

    leakBarSeries->clear();
    leakBarAxisX->clear();
    QStringList cats;
    QBarSet* set = new QBarSet("Leaks MB");
    for (auto it = leaksByFile.begin(); it != leaksByFile.end(); ++it) {
        cats << it.key();
        *set << toMB(it.value());
    }
    if (cats.isEmpty()) { cats << "(sin datos)"; *set << 0.0; }
    leakBarSeries->append(set);
    leakBarAxisX->append(cats);

    leakPieSeries->clear();
    for (auto it = leaksByFile.begin(); it != leaksByFile.end(); ++it) {
        leakPieSeries->append(it.key(), it.value());
    }

    leakTimeSeries->clear();
    for (const auto &p : leakTimesTmp->points()) {
        leakTimeSeries->append(p);
    }
    delete leakTimesTmp;
}

// ================== UI build ==================
void MainWindow::buildTabs() {
    // ----------- Vista General -----------
    tabOverview = new QWidget(this);
    auto *ovLay = new QVBoxLayout(tabOverview);
    auto *metrics = new QHBoxLayout();
    lblMemMB = new QLabel("Mem actual: 0 MB");
    lblLive  = new QLabel("Vivas: 0");
    lblLeakMB= new QLabel("Fugas: 0 MB");
    lblMaxMB = new QLabel("Máx usado: 0 MB");
    lblTotalAlloc = new QLabel("Total allocs: 0");
    metrics->addWidget(lblMemMB);  metrics->addSpacing(12);
    metrics->addWidget(lblLive);   metrics->addSpacing(12);
    metrics->addWidget(lblLeakMB); metrics->addSpacing(12);
    metrics->addWidget(lblMaxMB);  metrics->addSpacing(12);
    metrics->addWidget(lblTotalAlloc);
    metrics->addStretch(1);
    ovLay->addLayout(metrics);

    ovSeries = new QLineSeries(this);
    ovChart = new QChart();
    ovChart->addSeries(ovSeries);
    ovChart->setTitle("Uso de memoria (MB) vs tiempo (s)");
    ovChart->legend()->hide();
    ovAxisX = new QValueAxis(this);
    ovAxisY = new QValueAxis(this);
    ovAxisX->setTitleText("Tiempo (s)");
    ovAxisY->setTitleText("MB");
    ovAxisX->setRange(0, 60);
    ovAxisY->setRange(0, 1);
    ovChart->addAxis(ovAxisX, Qt::AlignBottom);
    ovChart->addAxis(ovAxisY, Qt::AlignLeft);
    ovSeries->attachAxis(ovAxisX);
    ovSeries->attachAxis(ovAxisY);
    ovChartView = new QChartView(ovChart, tabOverview);
    ovChartView->setRenderHint(QPainter::Antialiasing);
    ovLay->addWidget(ovChartView, 1);

    ovTop3 = new QTableWidget(0, 3, tabOverview);
    ovTop3->setHorizontalHeaderLabels(QStringList() << "Archivo" << "Asignaciones vivas" << "MB vivos");
    ovTop3->horizontalHeader()->setStretchLastSection(true);
    ovLay->addWidget(ovTop3);

    auto *demoRow = new QHBoxLayout();
    btnStartDemo = new QPushButton("Iniciar demo externa");
    btnStopDemo  = new QPushButton("Detener demo");
    btnStopDemo->setEnabled(false);
    demoRow->addWidget(btnStartDemo);
    demoRow->addWidget(btnStopDemo);
    demoRow->addStretch(1);
    ovLay->addLayout(demoRow);
    connect(btnStartDemo, &QPushButton::clicked, this, &MainWindow::onStartDemo);
    connect(btnStopDemo,  &QPushButton::clicked, this, &MainWindow::onStopDemo);

    tabs->addTab(tabOverview, "Vista General");

    // ----------- Mapa de Memoria -----------
    tabMap = new QWidget(this);
    auto *mapLay = new QVBoxLayout(tabMap);
    tblMap = new QTableWidget(0, 6, tabMap);
    tblMap->setHorizontalHeaderLabels(QStringList() << "Dirección" << "Bytes" << "Tipo" << "Archivo" << "Línea" << "t (ms)");
    tblMap->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mapLay->addWidget(tblMap);
    tabs->addTab(tabMap, "Mapa de Memoria");

    // ----------- Asignación por Archivo -----------
    tabByFile = new QWidget(this);
    auto *bfLay = new QVBoxLayout(tabByFile);
    bySeries = new QBarSeries(this);
    byChart = new QChart();
    byChart->addSeries(bySeries);
    byChart->setTitle("Asignaciones vivas por archivo (conteo)");
    byAxisX = new QBarCategoryAxis(this);
    byAxisY = new QValueAxis(this);
    byAxisY->setTitleText("Conteo");
    byChart->addAxis(byAxisX, Qt::AlignBottom);
    byChart->addAxis(byAxisY, Qt::AlignLeft);
    bySeries->attachAxis(byAxisX);
    bySeries->attachAxis(byAxisY);
    byChartView = new QChartView(byChart, tabByFile);
    byChartView->setRenderHint(QPainter::Antialiasing);
    bfLay->addWidget(byChartView);
    tabs->addTab(tabByFile, "Asignación por Archivo");

    // ----------- Memory Leaks -----------
    tabLeaks = new QWidget(this);
    auto *lkLay = new QVBoxLayout(tabLeaks);

    auto *lkTop = new QHBoxLayout();
    lblLeaksTotalMB = new QLabel("Total fuga: 0 MB");
    lblLeakLargest  = new QLabel("Leak más grande: N/A");
    lblLeakFileMost = new QLabel("Archivo con más leaks: N/A");
    lblLeakRate     = new QLabel("Tasa de leaks: 0%");
    lkTop->addWidget(lblLeaksTotalMB); lkTop->addSpacing(12);
    lkTop->addWidget(lblLeakLargest);  lkTop->addSpacing(12);
    lkTop->addWidget(lblLeakFileMost); lkTop->addSpacing(12);
    lkTop->addWidget(lblLeakRate);     lkTop->addStretch(1);
    lkLay->addLayout(lkTop);

    leakBarSeries = new QBarSeries(this);
    leakBarChart  = new QChart();
    leakBarChart->addSeries(leakBarSeries);
    leakBarChart->setTitle("Leaks por archivo (MB)");
    leakBarAxisX = new QBarCategoryAxis(this);
    leakBarAxisY = new QValueAxis(this);
    leakBarAxisY->setTitleText("MB");
    leakBarChart->addAxis(leakBarAxisX, Qt::AlignBottom);
    leakBarChart->addAxis(leakBarAxisY, Qt::AlignLeft);
    leakBarSeries->attachAxis(leakBarAxisX);
    leakBarSeries->attachAxis(leakBarAxisY);
    leakBarView = new QChartView(leakBarChart, tabLeaks);
    leakBarView->setRenderHint(QPainter::Antialiasing);
    lkLay->addWidget(leakBarView);

    leakPieSeries = new QPieSeries(this);
    leakPieChart  = new QChart();
    leakPieChart->addSeries(leakPieSeries);
    leakPieChart->setTitle("Distribución de leaks por archivo");
    leakPieView = new QChartView(leakPieChart, tabLeaks);
    leakPieView->setRenderHint(QPainter::Antialiasing);
    lkLay->addWidget(leakPieView);

    leakTimeSeries = new QLineSeries(this);
    leakTimeChart  = new QChart();
    leakTimeChart->addSeries(leakTimeSeries);
    leakTimeChart->setTitle("Temporal de detección de leaks (MB por asignación)");
    leakTimeAxisX = new QValueAxis(this);
    leakTimeAxisY = new QValueAxis(this);
    leakTimeAxisX->setTitleText("Tiempo (s)");
    leakTimeAxisY->setTitleText("MB");
    leakTimeChart->addAxis(leakTimeAxisX, Qt::AlignBottom);
    leakTimeChart->addAxis(leakTimeAxisY, Qt::AlignLeft);
    leakTimeSeries->attachAxis(leakTimeAxisX);
    leakTimeSeries->attachAxis(leakTimeAxisY);
    leakTimeView = new QChartView(leakTimeChart, tabLeaks);
    leakTimeView->setRenderHint(QPainter::Antialiasing);
    lkLay->addWidget(leakTimeView);

    tabs->addTab(tabLeaks, "Memory Leaks");
}

void MainWindow::updateOverview() {
    lblMemMB->setText(QString("Mem actual: %1 MB").arg(QString::number(toMB(liveBytes),'f',2)));
    lblLive->setText(QString("Vivas: %1").arg(alive.size()));
    lblLeakMB->setText(QString("Fugas: %1 MB").arg(QString::number(toMB(liveBytes),'f',2))); // se consolida al desconectar
    lblMaxMB->setText(QString("Máx usado: %1 MB").arg(QString::number(maxLiveMB,'f',2)));
    lblTotalAlloc->setText(QString("Total allocs: %1").arg(totalAllocs));
}

void MainWindow::updateTop3() {
    struct Row { QString file; qint64 cnt; qint64 bytes; };
    QList<Row> rows;
    for (auto it = aliveBytesByFile.begin(); it != aliveBytesByFile.end(); ++it) {
        rows.push_back({it.key(), aliveCountByFile.value(it.key()), it.value()});
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b){ return a.bytes > b.bytes; });

    const int rcount = std::min<int>(3, static_cast<int>(rows.size()));
    ovTop3->setRowCount(rcount);
    for (int i=0;i<rcount;++i) {
        const auto &r = rows[i];
        ovTop3->setItem(i,0,new QTableWidgetItem(r.file));
        ovTop3->setItem(i,1,new QTableWidgetItem(QString::number(r.cnt)));
        ovTop3->setItem(i,2,new QTableWidgetItem(QString::number(toMB(r.bytes),'f',2)));
    }
}

void MainWindow::updateMapAdd(const Alloc& a) {
    int row = tblMap->rowCount();
    tblMap->insertRow(row);
    rowOfPtr[a.ptr] = row;

    tblMap->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(QString::number(a.ptr, 16))));
    tblMap->setItem(row, 1, new QTableWidgetItem(QString::number(a.bytes)));
    tblMap->setItem(row, 2, new QTableWidgetItem(a.type));
    tblMap->setItem(row, 3, new QTableWidgetItem(a.file));
    tblMap->setItem(row, 4, new QTableWidgetItem(QString::number(a.line)));
    tblMap->setItem(row, 5, new QTableWidgetItem(QString::number(a.ts_ms)));
}

void MainWindow::updateMapRemove(quintptr ptr) {
    if (!rowOfPtr.contains(ptr)) return;
    int row = rowOfPtr.take(ptr);
    tblMap->removeRow(row);

    rowOfPtr.clear();
    for (int r=0;r<tblMap->rowCount();++r) {
        auto txt = tblMap->item(r,0)->text();
        bool ok=false;
        quintptr p = txt.mid(2).toULongLong(&ok, 16);
        if (ok) rowOfPtr[p] = r;
    }
}

void MainWindow::updateByFileChart() {
    bySeries->clear();
    byAxisX->clear();

    QStringList cats;
    QBarSet* set = new QBarSet("Asignaciones vivas");
    for (auto it = aliveCountByFile.begin(); it != aliveCountByFile.end(); ++it) {
        cats << it.key();
        *set << (double)it.value();
    }
    if (cats.isEmpty()) { cats << "(sin datos)"; *set << 0.0; }
    bySeries->append(set);
    byAxisX->append(cats);

    double ymax = 0.0;
    for (int i=0;i<set->count();++i) ymax = std::max(ymax, set->at(i));
    byAxisY->setRange(0, std::max(1.0, ymax*1.2));
}

// ================== Demo buttons ==================
void MainWindow::onStartDemo() {
    if (demoProc && demoProc->state()!=QProcess::NotRunning) return;

#ifdef Q_OS_WIN
    const QString exeName = "MemTargetDemo.exe";
#else
    const QString exeName = "MemTargetDemo";
#endif
    QString path = QCoreApplication::applicationDirPath() + "/" + exeName;

    demoProc = new QProcess(this);
    demoProc->setProgram(path);
    connect(demoProc, &QProcess::finished, this, &MainWindow::onDemoFinished);
    demoProc->start();
    btnStartDemo->setEnabled(false);
    btnStopDemo->setEnabled(true);
}

void MainWindow::onStopDemo() {
    if (!demoProc) return;
    demoProc->kill();
    demoProc->waitForFinished(1000);
}

void MainWindow::onDemoFinished(int, QProcess::ExitStatus) {
    btnStartDemo->setEnabled(true);
    btnStopDemo->setEnabled(false);
}










