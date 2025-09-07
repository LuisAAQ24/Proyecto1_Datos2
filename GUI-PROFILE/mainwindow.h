#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTableWidget>
#include <QHash>

// Qt Charts (clases sin prefijo de namespace)
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>

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

    // ---- Servidor de sockets ----
    QTcpServer* server = nullptr;
    QTcpSocket* client = nullptr;
    int serverPort = 5555;
    QByteArray rxBuffer;

    // ---- Datos (modelo en la GUI) ----
    struct Alloc {
        quintptr ptr;
        qint64   bytes;
        QString  type;   // "new" / "new[]"
        QString  file;
        int      line;
        qint64   ts_ms;  // timestamp ms (epoch)
    };
    QHash<quintptr, Alloc> alive;
    QHash<QString, qint64> aliveCountByFile;
    QHash<QString, qint64> aliveBytesByFile;

    qint64 totalAllocs = 0;
    qint64 liveBytes   = 0;
    qint64 maxLiveMB   = 0; // pico observado

    // ====== Tabs ======
    QTabWidget* tabs = nullptr;

    // ---- Pestaña: Vista General ----
    QWidget* tabOverview = nullptr;
    QLabel *lblMemMB = nullptr, *lblLive = nullptr, *lblLeakMB = nullptr,
                                                        *lblMaxMB = nullptr, *lblTotalAlloc = nullptr;

    QChartView*  ovChartView = nullptr;
    QChart*      ovChart     = nullptr;
    QLineSeries* ovSeries    = nullptr;
    QValueAxis*  ovAxisX     = nullptr;
    QValueAxis*  ovAxisY     = nullptr;
    double t0_ms = -1;

    QTableWidget* ovTop3 = nullptr;

    // ---- Pestaña: Mapa de memoria ----
    QWidget* tabMap = nullptr;
    QTableWidget* tblMap = nullptr;
    QHash<quintptr,int> rowOfPtr;

    // ---- Pestaña: Asignación por archivo ----
    QWidget* tabByFile = nullptr;
    QChartView* byChartView = nullptr;
    QChart*     byChart     = nullptr;
    QBarSeries* bySeries    = nullptr;
    QBarCategoryAxis* byAxisX = nullptr;
    QValueAxis*  byAxisY    = nullptr;

    // ---- Pestaña: Memory leaks ----
    QWidget* tabLeaks = nullptr;
    QLabel *lblLeaksTotalMB = nullptr, *lblLeakLargest = nullptr,
                                           *lblLeakFileMost = nullptr, *lblLeakRate = nullptr;

    QChartView* leakBarView = nullptr;   // barras por archivo
    QChart*     leakBarChart = nullptr;
    QBarSeries* leakBarSeries = nullptr;
    QBarCategoryAxis* leakBarAxisX = nullptr;
    QValueAxis* leakBarAxisY = nullptr;

    QChartView* leakPieView = nullptr;   // pie por archivo
    QChart*     leakPieChart = nullptr;
    QPieSeries* leakPieSeries = nullptr;

    QChartView* leakTimeView = nullptr;  // temporal de leaks detectados
    QChart*     leakTimeChart = nullptr;
    QLineSeries* leakTimeSeries = nullptr;
    QValueAxis*  leakTimeAxisX = nullptr;
    QValueAxis*  leakTimeAxisY = nullptr;

    // ---- Botones (demo) ----
    QPushButton* btnStartDemo = nullptr;
    QPushButton* btnStopDemo  = nullptr;
    QProcess*    demoProc     = nullptr;

    // ====== Métodos ======
    void startServer();
    void processLine(const QByteArray& line);
    void handleAlloc(const QJsonObject& o);
    void handleFree(const QJsonObject& o);
    void handleTick(const QJsonObject& o);
    void onClientDisconnectedComputeLeaks();

    // UI helpers
    void buildTabs();
    void updateOverview();
    void updateTop3();
    void updateMapAdd(const Alloc& a);
    void updateMapRemove(quintptr ptr);
    void updateByFileChart();
    void updateLeakChartsFromAlive(); // si lo necesitas más adelante

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

    void onStartDemo();
    void onStopDemo();
    void onDemoFinished(int, QProcess::ExitStatus);
};

#endif // MAINWINDOW_H




