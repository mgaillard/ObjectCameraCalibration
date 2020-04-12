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
		/*
		const ObjectPose pose{
			QVector3D(0.0, 0.0, 0.0),
			QVector3D(xAngleInitial, yAngleInitial, zAngleInitial)
		};
		
		const auto optimizedPose = runBundleAdjustment(ui.viewerWidget, targetImage, pose);

		qDebug() << "translation = " << optimizedPose.translation
			     << " rotation = " << optimizedPose.rotation;
		*/

		const ObjectPose optimizedPose{
			QVector3D(-0.14, -0.12, -0.17),
			QVector3D(31.50, 41.63, -31.09)
		};
		
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
