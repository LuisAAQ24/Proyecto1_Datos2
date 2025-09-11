#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHostAddress>
#include <QDebug>

// Qt Charts (macro de namespace por portabilidad)
#include <QtCharts/QChartGlobal>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>


// JSON
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// Layout / misc
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --- Socket servidor mínimo
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);

    const bool ok = m_server->listen(QHostAddress::LocalHost, 5050);
    statusBar()->showMessage(ok
                                 ? "Servidor escuchando en 127.0.0.1:5050"
                                 : "ERROR: no se pudo abrir el puerto 5050");

    // --- Inicializar la gráfica en la pestaña "Memory leaks" ---
    initLeaksChart();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::initLeaksChart() {
    // Serie vacía (se llenará al recibir fugas reales por socket)
    leaksSeries = new QLineSeries(this);

    // Chart y ejes
    leaksChart = new QChart();
    leaksChart->addSeries(leaksSeries);
    leaksChart->setTitle("Memory leaks");
    leaksChart->legend()->hide();
    leaksChart->createDefaultAxes();

    if (auto* axX = qobject_cast<QValueAxis*>(leaksChart->axes(Qt::Horizontal).value(0))) {
        axX->setTitleText("Leak # (índice)");
        axX->setLabelFormat("%.0f");
    }
    if (auto* axY = qobject_cast<QValueAxis*>(leaksChart->axes(Qt::Vertical).value(0))) {
        axY->setTitleText("Tamaño (bytes)");
        axY->setLabelFormat("%.0f");
        axY->setMin(0);
    }

    // Vista y render
    leaksChartView = new QChartView(leaksChart, this);
    leaksChartView->setRenderHint(QPainter::Antialiasing);

    // Insertar en el contenedor del .ui (widget con objectName="leaksChartContainer")
    QWidget* cont = ui->centralwidget->findChild<QWidget*>("leaksChartContainer");
    if (cont) {
        // Asegura layout interno del contenedor
        if (!cont->layout()) {
            auto* v = new QVBoxLayout();
            v->setContentsMargins(0,0,0,0);
            cont->setLayout(v);
        }
        // Inserta la vista y pide expansión
        leaksChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        leaksChartView->setMinimumSize(0,0);
        cont->layout()->addWidget(leaksChartView);


        QWidget* page = cont->parentWidget();
        if (page && !page->layout()) {
            auto* outer = new QVBoxLayout(page);
            outer->setContentsMargins(0,0,0,0);
            outer->addWidget(cont);
        }
    } else {
        qWarning() << "No existe leaksChartContainer en el .ui";
    }
}

void MainWindow::onNewConnection() {
    m_client = m_server->nextPendingConnection();
    connect(m_client, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    statusBar()->showMessage("Cliente conectado");
}

void MainWindow::onReadyRead() {
    while (m_client && m_client->canReadLine()) {
        const QByteArray line = m_client->readLine().trimmed();
        // Esperamos algo como: {"leaks":[{"ptr":"0x...","size":N}, ...]}
        const auto doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) {
            qWarning() << "JSON inválido" << line;
            continue;
        }

        const auto leaks = doc.object().value("leaks").toArray();

        // Limpiar y graficar: X = índice (1..N), Y = size en bytes
        leaksSeries->clear();
        long long totalBytes = 0;
        for (int i = 0; i < leaks.size(); ++i) {
            const auto o = leaks[i].toObject();
            const double size = o.value("size").toDouble(0);
            totalBytes += static_cast<long long>(size);
            leaksSeries->append(i + 1, size);
        }

        // Título con resumen real
        if (leaksChart) {
            leaksChart->setTitle(QString("Memory leaks: %1 fugas, %2 bytes")
                                     .arg(leaks.size()).arg(totalBytes));
        }

        // Reescala eje Y simple
        if (auto* axY = qobject_cast<QValueAxis*>(leaksChart->axes(Qt::Vertical).value(0))) {
            const auto pts = leaksSeries->pointsVector();
            const double ymax = pts.isEmpty() ? 1.0 : std::max(1.0, pts.last().y() * 1.1);
            axY->setMax(ymax);
        }

        qDebug() << "[GUI] graficadas" << leaks.size() << "fugas";
    }
}







