#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QBarSeries>
#include <QPieSeries>
#include <QTimer>
#include <QTableWidget>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void actualizarMetricasGenerales();
    void actualizarLineaTiempo();
    void actualizarMapaMemoria();
    void actualizarAsignacionPorArchivo();
    void actualizarMemoryLeaks();
    void procesarDatosSocket(const QJsonObject& datos);

private slots:
    void onTimerTimeout();

private:
    void setupVistaGeneral();
    void setupMapaMemoria();
    void setupAsignacionArchivo();
    void setupMemoryLeaks();
    void actualizarPanelMetricas();
    void actualizarResumenTopArchivos();
    void actualizarGraficosLeaks(const QMap<QString, int>& conteoPorArchivo);

    Ui::MainWindow *ui;
    QTimer *timerActualizacion;

    // Componentes para gráficos
    QChart *chartLineaTiempo;
    QLineSeries *seriesMemoria;

    QChart *chartBarrasLeaks;
    QBarSeries *seriesBarrasLeaks;

    QChart *chartPieLeaks;
    QPieSeries *seriesPieLeaks;

    // Datos en memoria
    qint64 tiempoInicio;
    QVector<QPair<qint64, qint64>> datosMemoria; // tiempo, memoria
    QMap<QString, qint64> asignacionesPorArchivo;
    QList<QJsonObject> leaksDetectados;

    // Métricas actuales
    qint64 memoriaActual;
    qint64 maxMemoriaUsada;
    qint64 totalAsignaciones;
    qint64 totalLiberaciones;
    qint64 memoriaFugada;
};

#endif // MAINWINDOW_H
