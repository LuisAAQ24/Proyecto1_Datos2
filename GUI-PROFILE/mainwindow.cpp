#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ListaGuardado.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QValueAxis>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QHeaderView>
#include <QDateTime>
#include <QBrush>
#include <QColor>
#include <map> // Necesario para agrupar datos

class ServidorSocket;
extern ListaGuardado listaGlobal;
extern ServidorSocket* servidorSocket;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , memoriaActual(0)
    , maxMemoriaUsada(0)
    , totalAsignaciones(0)
    , totalLiberaciones(0)
    , memoriaFugada(0)
{
    ui->setupUi(this);

    tiempoInicio = QDateTime::currentMSecsSinceEpoch();

    // Configurar timer para actualizaciones periódicas
    timerActualizacion = new QTimer(this);
    connect(timerActualizacion, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
    timerActualizacion->start(1000); // Actualizar cada segundo

    // Configurar las pestañas
    setupVistaGeneral();
    setupMapaMemoria();
    setupAsignacionArchivo();
    setupMemoryLeaks();

    // Configurar ventana
    setWindowTitle("Memory Profiler - GUI");
    resize(1200, 800);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupVistaGeneral()
{

    // Configurar gráfico de línea de tiempo
    chartLineaTiempo = new QChart();
    seriesMemoria = new QLineSeries();
    seriesMemoria->setName("Uso de Memoria (MB)");

    chartLineaTiempo->addSeries(seriesMemoria);
    chartLineaTiempo->setTitle("Línea de Tiempo - Uso de Memoria");
    chartLineaTiempo->setAnimationOptions(QChart::SeriesAnimations);

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Tiempo (segundos)");
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Memoria (MB)");

    chartLineaTiempo->addAxis(axisX, Qt::AlignBottom);
    chartLineaTiempo->addAxis(axisY, Qt::AlignLeft);
    seriesMemoria->attachAxis(axisX);
    seriesMemoria->attachAxis(axisY);

    ui->chartViewLineaTiempo->setChart(chartLineaTiempo);
    ui->chartViewLineaTiempo->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::setupMapaMemoria()
{
    // Configurar tabla del mapa de memoria
    QStringList headers;
    headers << "Dirección" << "Tamaño (bytes)" << "Tipo" << "Archivo" << "Timestamp";
    ui->tableMapaMemoria->setColumnCount(5);
    ui->tableMapaMemoria->setHorizontalHeaderLabels(headers);
    ui->tableMapaMemoria->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableMapaMemoria->setAlternatingRowColors(true);
}

void MainWindow::setupAsignacionArchivo()
{
    // Configurar tabla de asignación por archivo
    QStringList headers;
    headers << "Archivo" << "Conteo Asignaciones" << "Memoria Total (MB)";
    ui->tableAsignacionArchivo->setColumnCount(3);
    ui->tableAsignacionArchivo->setHorizontalHeaderLabels(headers);
    ui->tableAsignacionArchivo->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::setupMemoryLeaks()
{
    // Configurar gráfico de barras para leaks por archivo
    chartBarrasLeaks = new QChart();
    seriesBarrasLeaks = new QBarSeries();
    chartBarrasLeaks->addSeries(seriesBarrasLeaks);
    chartBarrasLeaks->setTitle("Leaks por Archivo");
    chartBarrasLeaks->setAnimationOptions(QChart::SeriesAnimations);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    chartBarrasLeaks->addAxis(axisX, Qt::AlignBottom);
    seriesBarrasLeaks->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Número de Leaks");
    chartBarrasLeaks->addAxis(axisY, Qt::AlignLeft);
    seriesBarrasLeaks->attachAxis(axisY);

    ui->chartViewBarrasLeaks->setChart(chartBarrasLeaks);
    ui->chartViewBarrasLeaks->setRenderHint(QPainter::Antialiasing);

    // Configurar gráfico de pie
    chartPieLeaks = new QChart();
    seriesPieLeaks = new QPieSeries();
    chartPieLeaks->addSeries(seriesPieLeaks);
    chartPieLeaks->setTitle("Distribución de Leaks");
    chartPieLeaks->legend()->setVisible(true);
    chartPieLeaks->legend()->setAlignment(Qt::AlignRight);

    ui->chartViewPieLeaks->setChart(chartPieLeaks);
    ui->chartViewPieLeaks->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::onTimerTimeout()
{
    // Esta función ahora actualiza toda la UI desde la fuente de datos global.
    actualizarMetricasGenerales();
    actualizarMapaMemoria();
    actualizarAsignacionPorArchivo();
    actualizarMemoryLeaks();
}

void MainWindow::actualizarMetricasGenerales()
{
    actualizarPanelMetricas();
    actualizarLineaTiempo();
    actualizarResumenTopArchivos();
}

void MainWindow::actualizarPanelMetricas()
{
    QJsonObject metricas = listaGlobal.obtenerMetricas();
    memoriaActual = metricas["memoriaActual"].toInt();
    maxMemoriaUsada = metricas["maxMemoriaUsada"].toInt();
    totalAsignaciones = metricas["totalAsignaciones"].toInt();
    totalLiberaciones = metricas["totalLiberaciones"].toInt();

    ui->labelMemoriaActual->setText(QString("%1 MB").arg(memoriaActual / 1024.0 / 1024.0, 0, 'f', 2));
    ui->labelAsignacionesActivas->setText(QString::number(totalAsignaciones - totalLiberaciones));
    ui->labelMemoriaFugada->setText(QString("%1 MB").arg(memoriaFugada / 1024.0 / 1024.0, 0, 'f', 2));
    ui->labelMaxMemoria->setText(QString("%1 MB").arg(maxMemoriaUsada / 1024.0 / 1024.0, 0, 'f', 2));
    ui->labelTotalAsignaciones->setText(QString::number(totalAsignaciones));
}

void MainWindow::actualizarLineaTiempo()
{
    qint64 tiempoActual = QDateTime::currentMSecsSinceEpoch() - tiempoInicio;
    qint64 segundos = tiempoActual / 1000;

    seriesMemoria->append(segundos, memoriaActual / 1024.0 / 1024.0);

    if (seriesMemoria->count() > 60) {
        seriesMemoria->remove(0);
    }

    chartLineaTiempo->axes(Qt::Horizontal).first()->setRange(qMax(0.0, static_cast<double>(segundos - 60)), segundos);
    chartLineaTiempo->axes(Qt::Vertical).first()->setRange(0, qMax(0.1, (maxMemoriaUsada / 1024.0 / 1024.0) * 1.1));
}

void MainWindow::actualizarResumenTopArchivos()
{
    QLayoutItem* child;
    while ((child = ui->layoutResumenArchivos->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    // Agrupar datos directamente desde listaGlobal
    std::map<std::string, std::pair<int, size_t>> resumen;
    Guardado* actual = listaGlobal.getInicio();
    while (actual) {
        resumen[actual->archivo].first++;
        resumen[actual->archivo].second += actual->tamano;
        actual = actual->siguiente;
    }

    QList<QPair<QString, qint64>> topArchivos;
    for (const auto& pair : resumen) {
        topArchivos.append(qMakePair(QString::fromStdString(pair.first), pair.second.second));
    }

    std::sort(topArchivos.begin(), topArchivos.end(),
              [](const QPair<QString, qint64>& a, const QPair<QString, qint64>& b) {
                  return a.second > b.second;
              });

    for (int i = 0; i < qMin(3, topArchivos.size()); i++) {
        QString archivo = topArchivos[i].first;
        qint64 memoria = topArchivos[i].second;
        int conteo = resumen[archivo.toStdString()].first;

        QLabel *labelArchivo = new QLabel(this);
        labelArchivo->setText(QString("%1: %2 asignaciones, %3 MB")
                                  .arg(archivo)
                                  .arg(conteo)
                                  .arg(memoria / 1024.0 / 1024.0, 0, 'f', 2));
        ui->layoutResumenArchivos->addWidget(labelArchivo);
    }
}


void MainWindow::actualizarMapaMemoria()
{
    ui->tableMapaMemoria->setRowCount(0);
    Guardado* actual = listaGlobal.getInicio();
    int row = 0;
    while(actual) {
        QString archivo = QString::fromStdString(actual->archivo);
        if (archivo.startsWith("Unknown (lib/STL)")) {
            actual = actual->siguiente;
            continue; // Saltar esta asignación y pasar a la siguiente
        }
        ui->tableMapaMemoria->insertRow(row);
        ui->tableMapaMemoria->setItem(row, 0, new QTableWidgetItem(QString("0x%1").arg(reinterpret_cast<quintptr>(actual->direccion), 16, 16, QChar('0'))));
        ui->tableMapaMemoria->setItem(row, 1, new QTableWidgetItem(QString::number(actual->tamano)));
        ui->tableMapaMemoria->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(actual->tipo)));
        ui->tableMapaMemoria->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(actual->archivo)));
        ui->tableMapaMemoria->setItem(row, 4, new QTableWidgetItem(QDateTime::fromSecsSinceEpoch(actual->marcaDeTiempo).toString()));
        actual = actual->siguiente;
        row++;
    }
}

void MainWindow::actualizarAsignacionPorArchivo()
{
    // Agrupar datos directamente desde listaGlobal
    std::map<std::string, std::pair<int, size_t>> resumen;
    Guardado* actual = listaGlobal.getInicio();
    while (actual) {
        QString archivo = QString::fromStdString(actual->archivo);
        if (archivo.startsWith("Unknown (lib/STL)")) {
            actual = actual->siguiente;
            continue; // Saltar esta asignación y pasar a la siguiente
        }
        resumen[actual->archivo].first++;
        resumen[actual->archivo].second += actual->tamano;
        actual = actual->siguiente;
    }

    ui->tableAsignacionArchivo->setRowCount(0); // Limpiar
    int row = 0;
    for (const auto& pair : resumen) {
        ui->tableAsignacionArchivo->insertRow(row);
        ui->tableAsignacionArchivo->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(pair.first)));
        ui->tableAsignacionArchivo->setItem(row, 1, new QTableWidgetItem(QString::number(pair.second.first)));
        ui->tableAsignacionArchivo->setItem(row, 2, new QTableWidgetItem(QString::number(pair.second.second / 1024.0 / 1024.0, 'f', 2)));
        row++;
    }
}


void MainWindow::actualizarMemoryLeaks()
{
    auto fugas = listaGlobal.reportLeaks();
    memoriaFugada = 0; // Reiniciar para recalcular

    if (!fugas.empty()) {
        qint64 leakMasGrande = 0;
        QString archivoLeakMasGrande;
        QMap<QString, int> conteoPorArchivo;

        for (const auto& leak : fugas) {
            memoriaFugada += leak.tamano;

            if (leak.tamano > leakMasGrande) {
                leakMasGrande = leak.tamano;
                archivoLeakMasGrande = QString::fromStdString(leak.archivo);
            }

            QString archivo = QString::fromStdString(leak.archivo);
            conteoPorArchivo[archivo]++;
        }
        //qDebug() << "Conteo de Leaks por Archivo:" << conteoPorArchivo;

        QString archivoMasLeaks;
        int maxLeaks = 0;
        for (auto it = conteoPorArchivo.begin(); it != conteoPorArchivo.end(); ++it) {
            if (it.value() > maxLeaks) {
                maxLeaks = it.value();
                archivoMasLeaks = it.key();
            }
        }

        ui->labelTotalFugado->setText(QString("%1 MB").arg(memoriaFugada / 1024.0 / 1024.0, 0, 'f', 2));
        ui->labelLeakMasGrande->setText(QString("%1 bytes en %2").arg(leakMasGrande).arg(archivoLeakMasGrande));
        ui->labelArchivoMasLeaks->setText(archivoMasLeaks);
        double tasaLeaks = totalAsignaciones > 0 ? (double)fugas.size() / totalAsignaciones : 0.0;
        ui->labelTasaLeaks->setText(QString("%1%").arg(tasaLeaks * 100, 0, 'f', 1));

        actualizarGraficosLeaks(conteoPorArchivo);
    } else {
        // Limpiar UI si no hay leaks
        ui->labelTotalFugado->setText("0 MB");
        ui->labelLeakMasGrande->setText("0 bytes");
        ui->labelArchivoMasLeaks->setText("N/A");
        ui->labelTasaLeaks->setText("0%");
        seriesBarrasLeaks->clear();
        seriesPieLeaks->clear();
    }
}


void MainWindow::actualizarGraficosLeaks(const QMap<QString, int>& conteoPorArchivo)
{
    seriesBarrasLeaks->clear();
    seriesPieLeaks->clear();

    QBarSet *setBarras = new QBarSet("Leaks");
    QStringList categorias;
    for (auto it = conteoPorArchivo.begin(); it != conteoPorArchivo.end(); ++it) {
        *setBarras << it.value();
        categorias << it.key();
        seriesPieLeaks->append(it.key(), it.value());
    }
    seriesBarrasLeaks->append(setBarras);
    int maxLeaks = 0;
    if (!conteoPorArchivo.isEmpty()) {
        maxLeaks = *std::max_element(conteoPorArchivo.constBegin(), conteoPorArchivo.constEnd());
    }


    QValueAxis *axisY = qobject_cast<QValueAxis*>(chartBarrasLeaks->axes(Qt::Vertical).first());

    if (axisY && maxLeaks > 0) {
        axisY->setRange(0, maxLeaks * 1.1);
    }
    QBarCategoryAxis *axisX = qobject_cast<QBarCategoryAxis*>(chartBarrasLeaks->axes(Qt::Horizontal).first());
    if (axisX) {
        axisX->setCategories(categorias);
    }
}





