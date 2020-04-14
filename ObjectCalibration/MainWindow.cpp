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
		const double xInitial = QInputDialog::getDouble(this, 
			                                            tr("Input X translation"),
			                                            tr("Translation X"),
			                                            0.0,
			                                            -0.2,
			                                            0.2,
			                                            2);

		const double yInitial = QInputDialog::getDouble(this, 
			                                            tr("Input Y translation"),
			                                            tr("Translation Y"),
			                                            0.0,
			                                            -0.2,
			                                            0.2,
			                                            2);

		const double zInitial = QInputDialog::getDouble(this, 
			                                            tr("Input Z translation"),
			                                            tr("Translation Z"),
			                                            0.0,
			                                            -0.2,
			                                            0.2,
			                                            2);
		
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
		
		const ObjectPose pose{
			QVector3D(xInitial, yInitial, zInitial),
			QVector3D(xAngleInitial, yAngleInitial, zAngleInitial)
		};
		
		const auto optimizedPose = runBundleAdjustment(ui.viewerWidget, targetImage, pose);

		qDebug() << "translation = " << optimizedPose.translation
			     << " rotation = " << optimizedPose.rotation;

		/*
		const ObjectPose optimizedPose{
			QVector3D(-0.16, -0.10, 0.11),
			QVector3D(16.49, -32.47, 0.00)
		};
		*/
		
		// Best optimization
		const auto optimizedImage = ui.viewerWidget->renderToImage(optimizedPose);
		optimizedImage.save("image_optimized.png");
	}
}

void MainWindow::setupUi()
{
	ui.setupUi(this);

	connect(ui.actionRender, &QAction::triggered, this, &MainWindow::render);
}
