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
	
	runBundleAdjustment(ui.viewerWidget, targetImage, -30, 10, -5);

	// Ground-truth
	// ui.viewerWidget->renderAndComputeSimilarity(-30, 10, -5);

	// Best optimization
	// ui.viewerWidget->renderAndComputeSimilarity(-29.7888, 9.73193, -5);
}

void MainWindow::setupUi()
{
	ui.setupUi(this);

	connect(ui.actionRender, &QAction::triggered, this, &MainWindow::render);
}
