#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Crear el QTabWidget
    QTabWidget* tabs = new QTabWidget(this);

    // Crear widgets para cada pestaña
    QWidget* vistaGeneral = new QWidget();
    QWidget* mapaMemoria = new QWidget();
    QWidget* asignacionArchivo = new QWidget();
    QWidget* memoryLeaks = new QWidget();

    // Layouts con etiquetas de ejemplo
    QVBoxLayout* layoutVista = new QVBoxLayout();
    layoutVista->addWidget(new QLabel("Vista General"));
    vistaGeneral->setLayout(layoutVista);

    QVBoxLayout* layoutMapa = new QVBoxLayout();
    layoutMapa->addWidget(new QLabel("Mapa de Memoria"));
    mapaMemoria->setLayout(layoutMapa);

    QVBoxLayout* layoutArchivo = new QVBoxLayout();
    layoutArchivo->addWidget(new QLabel("Asignación por Archivo"));
    asignacionArchivo->setLayout(layoutArchivo);

    QVBoxLayout* layoutLeaks = new QVBoxLayout();
    layoutLeaks->addWidget(new QLabel("Memory Leaks"));
    memoryLeaks->setLayout(layoutLeaks);

    // Agregar las pestañas al QTabWidget
    tabs->addTab(vistaGeneral, "Vista General");
    tabs->addTab(mapaMemoria, "Mapa de Memoria");
    tabs->addTab(asignacionArchivo, "Asignación por Archivo");
    tabs->addTab(memoryLeaks, "Memory Leaks");

    // Poner el QTabWidget como widget central
    setCentralWidget(tabs);

    resize(800, 600);
}

MainWindow::~MainWindow()
{
    delete ui;
}



