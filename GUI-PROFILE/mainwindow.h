#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

// Qt Charts (definiciones completas + macro de namespace)
#include <QtCharts/QChartGlobal>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>


namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    // --- Socket servidor (se conserva tu lógica) ---
    QTcpServer* m_server = nullptr;
    QTcpSocket* m_client = nullptr;

    // --- Gráfica de "Memory leaks" ---
    QChartView*  leaksChartView = nullptr;
    QLineSeries* leaksSeries    = nullptr;
    QChart*      leaksChart     = nullptr;

    // Inicializa la vista y el chart
    void initLeaksChart();

private slots:
    void onNewConnection();
    void onReadyRead();
};

#endif // MAINWINDOW_H


