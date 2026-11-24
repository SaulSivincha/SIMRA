#pragma once
#include "simra/core.hpp"
#include <QMainWindow>
class QGraphicsScene;class QGraphicsView;class QComboBox;class QTableWidget;class QTextEdit;class QLabel;class QPushButton;class QTimer;
namespace simra {
class MainWindow final : public QMainWindow {
    Scenario scenario_;std::unique_ptr<SimulationEngine> engine_;
    QGraphicsScene* scene_{};QGraphicsView* view_{};QComboBox* scenarios_{};QTableWidget* robots_{};QTableWidget* orders_{};QTextEdit* events_{};QLabel* metrics_{};QLabel* status_{};QTimer* timer_{};
    QPushButton* start_{},*pause_{},*stop_{};uint64_t lastEvent_{};int nextOrder_{1};
    void render(const Snapshot&);void refresh();void loadPreset();void startSimulation();void stopSimulation();void addOrder();
public: explicit MainWindow(QWidget* parent=nullptr);
};
}
