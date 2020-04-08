#include "MainWindow.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "BundleAdjustment.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setupUi();
}

void MainWindow::render()
{
	// Ask the user for a file to import
	const QString filename = QFileDialog::getOpenFileName(this,
		                                                  tr("Load an image"),
		                                                  "",
		                                                  tr("Images (*.png *.jpg)"));

	// Check if file exists
	if (QFileInfo::exists(filename))
	{
		const QImage targetImage(filename);

		// Ask for initial configuration
		const double xAngleInitial = QInputDialog::getDouble(this, 
			                                                 tr("Input Euler X angle"),
			                                                 tr("Angle X"),
			                                                 0.0,
			                                                 -45.0,
			                                                 45.0,
			                                                 2);

		const double yAngleInitial = QInputDialog::getDouble(this, 
			                                                 tr("Input Euler Y angle"),
			                                                 tr("Angle Y"),
			                                                 0.0,
			                                                 -45.0,
			                                                 45.0,
			                                                 2);

		const double zAngleInitial = QInputDialog::getDouble(this, 
			                                                 tr("Input Euler Z angle"),
			                                                 tr("Angle Z"),
			                                                 0.0,
			                                                 -45.0,
			                                                 45.0,
			                                                 2);

		const auto result = runBundleAdjustment(ui.viewerWidget, targetImage, xAngleInitial, yAngleInitial, zAngleInitial);
		
		// Best optimization
		const auto optimized = ui.viewerWidget->renderToImage(result.x(), result.y(), result.z());
		optimized.save("image_optimized.png");
	}

	// TODO: Compute similarity in compute shader
}

void MainWindow::setupUi()
{
	ui.setupUi(this);

	connect(ui.actionRender, &QAction::triggered, this, &MainWindow::render);
}
