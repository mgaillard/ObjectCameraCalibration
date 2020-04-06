#include "MainWindow.h"

#include "BundleAdjustment.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setupUi();
}

void MainWindow::render()
{
	const QImage targetImage(":/MainWindow/Resources/target.png");
	
	const auto result = runBundleAdjustment(ui.viewerWidget, targetImage, -29.5, 10, -5);

	// Ground-truth
	const auto groundTruth = ui.viewerWidget->render(-30, 10, -5);
	groundTruth.save("image_groundtruth.png");

	// Best optimization
	const auto optimized = ui.viewerWidget->render(result.x(), result.y(), result.z());
	optimized.save("image_optimized.png");
}

void MainWindow::setupUi()
{
	ui.setupUi(this);

	connect(ui.actionRender, &QAction::triggered, this, &MainWindow::render);
}
