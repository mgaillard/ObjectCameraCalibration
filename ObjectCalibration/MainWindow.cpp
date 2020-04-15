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
	float avgMaxTranslationError = 0.0;
	float avgMaxRotationError = 0.0;
	int imageNumber = 0;
	
	// Automatic evaluation of optimization
	const auto inputDir = QFileDialog::getExistingDirectory(this, tr("Load images in a directory"), "");
	const QDir directory(inputDir);
	
	// Check if directory exists
	if (directory.exists())
	{
		const auto fileList = directory.entryInfoList(QStringList() << "*.png", QDir::Files);

		for (const auto& file : fileList)
		{
			// Read parameters in txt files
			const auto trueParametersFile = file.completeBaseName() + ".txt";
			const auto predParametersFile = file.completeBaseName() + "_pred.txt";
			if (directory.exists(trueParametersFile) && directory.exists(predParametersFile))
			{
				const auto truePose = readPose(directory.absoluteFilePath(trueParametersFile));
				const auto predPose = readPose(directory.absoluteFilePath(predParametersFile));

				const QImage targetImage(file.canonicalFilePath());
				
				const auto optimPose = runBundleAdjustment(ui.viewerWidget, targetImage, predPose);

				// Compare optimized pose to ground truth
				const auto translationError = maxTranslationError(truePose, optimPose);
				const auto rotationError = maxRotationError(truePose, optimPose);

				// Compute statistics
				avgMaxTranslationError += translationError;
				avgMaxRotationError += rotationError;
				imageNumber += 1;

				// Display error for this image
				qDebug() << file.fileName() << " " << translationError << " " << rotationError;

				// TODO: Render each image and the error maps in a separate folder
			}
		}
	}

	avgMaxTranslationError /= float(imageNumber);
	avgMaxRotationError /= float(imageNumber);

	qInfo() << "Number of images: " << imageNumber;
	qInfo() << "Average infinity norm for translations: " << avgMaxTranslationError;
	qInfo() << "Average infinity norm for rotations: " << avgMaxRotationError;
}

void MainWindow::setupUi()
{
	ui.setupUi(this);

	connect(ui.actionRender, &QAction::triggered, this, &MainWindow::render);
}
