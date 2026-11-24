#pragma once
#include <QWidget>
class QLabel;
class QProgressBar;
class QTableWidget;
namespace simra {
class ComparisonPanel final : public QWidget {
  QLabel *description_{}, *status_{};
  QTableWidget *table_{};
  QProgressBar *completedA_{}, *completedB_{}, *conflictsA_{}, *conflictsB_{};

public:
  explicit ComparisonPanel(QWidget *parent = nullptr);
  void showRunning();
  void showError(const QString &message);
  void showResult(const QByteArray &json);
};
} // namespace simra
