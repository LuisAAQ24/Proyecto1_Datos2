#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    auto *rootLayout = new QVBoxLayout(ui->centralwidget);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    tabs = new QTabWidget(this);
    rootLayout->addWidget(tabs, 1);

    setupLeaksTab();

    // La GUI NO se mide a sí misma
    resize(1100, 700);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupLeaksTab() {
    tabLeaks = new QWidget(this);
    auto *layout = new QVBoxLayout(tabLeaks);
    layout->setContentsMargins(8,8,8,8);
    layout->setSpacing(8);

    // Controles arriba
    auto *topRow = new QHBoxLayout();
    btnStartDemo = new QPushButton("Iniciar demo externa", tabLeaks);
    btnStopDemo  = new QPushButton("Detener demo", tabLeaks);
    btnStopDemo->setEnabled(false);

    lblVivas = new QLabel("Vivas: 0", tabLeaks);
    lblBytes = new QLabel("Bytes vivos: 0", tabLeaks);

    topRow->addWidget(btnStartDemo);
    topRow->addWidget(btnStopDemo);
    topRow->addStretch(1);
    topRow->addWidget(lblVivas);
    topRow->addSpacing(16);
    topRow->addWidget(lblBytes);
    layout->addLayout(topRow);

    // Series (sin prefijo de namespace)
    seriesLive  = new QLineSeries(this);
    seriesBytes = new QLineSeries(this);
    seriesLive->setName("Live (asignaciones)");
    seriesBytes->setName("Bytes vivos");

    // Chart
    chart = new QChart();
    chart->addSeries(seriesLive);
    chart->addSeries(seriesBytes);
    chart->setTitle("Monitoreo de memoria (proceso externo)");
    chart->legend()->setVisible(true);

    axisX = new QValueAxis(this);
    axisX->setTitleText("Tiempo (s)");
    axisX->setRange(0, xWindowSec);

    axisY = new QValueAxis(this);
    axisY->setTitleText("Conteo / Bytes");
    axisY->setRange(0, 10);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    seriesLive->attachAxis(axisX);
    seriesBytes->attachAxis(axisX);
    seriesLive->attachAxis(axisY);
    seriesBytes->attachAxis(axisY);

    chartView = new QChartView(chart, tabLeaks);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView, 1);

    // Conexiones de botones
    connect(btnStartDemo, &QPushButton::clicked, this, &MainWindow::onStartDemo);
    connect(btnStopDemo,  &QPushButton::clicked, this, &MainWindow::onStopDemo);

    tabs->addTab(tabLeaks, "Memory Leaks");
}

void MainWindow::onStartDemo() {
    if (proc && proc->state() != QProcess::NotRunning) return;

    // Limpia series
    seriesLive->clear();
    seriesBytes->clear();
    axisX->setRange(0, xWindowSec);
    axisY->setRange(0, 10);

#ifdef Q_OS_WIN
    const QString exeName = "MemTargetDemo.exe";
#else
    const QString exeName = "MemTargetDemo";
#endif
    QString path = QCoreApplication::applicationDirPath() + "/" + exeName;

    proc = new QProcess(this);
    proc->setProgram(path);

    connect(proc, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcReadyRead);
    connect(proc, &QProcess::finished, this, &MainWindow::onProcFinished);

    proc->start();
    if (!proc->waitForStarted(2000)) {
        delete proc; proc = nullptr;
        return;
    }
    btnStartDemo->setEnabled(false);
    btnStopDemo->setEnabled(true);
}

void MainWindow::onStopDemo() {
    if (!proc) return;
    proc->kill();
    proc->waitForFinished(1000);
}

void MainWindow::onProcReadyRead() {
    if (!proc) return;
    while (proc->canReadLine()) {
        const QByteArray line = proc->readLine().trimmed();
        if (!line.startsWith("MEMPROF")) continue;

        const QList<QByteArray> parts = line.split(' ');
        if (parts.size() < 5) continue;

        auto takeVal = [&](const char* key)->double {
            for (const auto& p : parts) {
                if (p.startsWith(key)) {
                    auto eq = p.indexOf('=');
                    if (eq > 0) {
                        bool ok = false;
                        double v = QString::fromLatin1(p.mid(eq+1)).toDouble(&ok);
                        if (ok) return v;
                    }
                }
            }
            return 0.0;
        };

        const double t_ms = takeVal("t_ms");
        const double live = takeVal("live");
        const double byts = takeVal("bytes");
        const double t_s  = t_ms / 1000.0;

        seriesLive->append(t_s, live);
        seriesBytes->append(t_s, byts);

        // Ventana deslizante
        if (t_s > xWindowSec) axisX->setRange(t_s - xWindowSec, t_s);

        // Ajuste eje Y
        double ymax = axisY->max();
        if (live > ymax) ymax = live;
        if (byts > ymax) ymax = byts;
        if (ymax < 10) ymax = 10;
        axisY->setRange(0, ymax * 1.1);

        lblVivas->setText(QString("Vivas: %1").arg((long long)live));
        lblBytes->setText(QString("Bytes vivos: %1").arg((long long)byts));
    }
}

void MainWindow::onProcFinished(int, QProcess::ExitStatus) {
    btnStartDemo->setEnabled(true);
    btnStopDemo->setEnabled(false);
}






