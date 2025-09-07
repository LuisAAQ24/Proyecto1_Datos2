#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QProcess>

// Qt Charts (incluye los headers de las clases que usamos)
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    // Tabs
    QTabWidget* tabs = nullptr;
    QWidget* tabLeaks = nullptr;

    // Gráfico (series de "live" y "bytes")
    QChartView*  chartView  = nullptr;
    QChart*      chart      = nullptr;
    QLineSeries* seriesLive = nullptr;
    QLineSeries* seriesBytes= nullptr;
    QValueAxis*  axisX      = nullptr;
    QValueAxis*  axisY      = nullptr;

    // (La GUI NO se mide a sí misma)
    QLabel* lblVivas = nullptr;
    QLabel* lblBytes = nullptr;

    // Proceso externo (programa medible)
    QProcess*    proc         = nullptr;
    QPushButton* btnStartDemo = nullptr;
    QPushButton* btnStopDemo  = nullptr;

    // Estado eje X
    double xWindowSec = 60.0;

    void setupLeaksTab();

private slots:
    void onStartDemo();
    void onStopDemo();
    void onProcReadyRead();
    void onProcFinished(int code, QProcess::ExitStatus st);
};

#endif // MAINWINDOW_H



