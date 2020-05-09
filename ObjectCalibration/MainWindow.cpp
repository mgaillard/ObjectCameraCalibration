#include "MainWindow.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "BundleAdjustment.h"
#include "Similarity.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setupUi();
}

void MainWindow::render()
{
	float predAvgMaxTranslationError = 0.0;
	float predAvgMaxRotationError = 0.0;
	float predAvgTranslationError = 0.0;
	float predAvgRotationError = 0.0;
	float optimAvgMaxTranslationError = 0.0;
	float optimAvgMaxRotationError = 0.0;
	float optimAvgTranslationError = 0.0;
	float optimAvgRotationError = 0.0;
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
				qInfo() << "Processing: " << file.fileName();
				
				const auto truePose = readPose(directory.absoluteFilePath(trueParametersFile));
				const auto predPose = readPose(directory.absoluteFilePath(predParametersFile));

				const QImage targetImage(file.canonicalFilePath());
				
				const auto optimPose = runBundleAdjustment(ui.viewerWidget, targetImage, predPose);
				// const auto optimPose = predPose;

				// Compare optimized pose to ground truth
				predAvgMaxTranslationError += maxTranslationError(truePose, predPose);
				predAvgMaxRotationError += maxRotationError(truePose, predPose);
				predAvgTranslationError += translationError(truePose, predPose);
				predAvgRotationError += rotationError(truePose, predPose);
				optimAvgMaxTranslationError += maxTranslationError(truePose, optimPose);
				optimAvgMaxRotationError += maxRotationError(truePose, optimPose);
				optimAvgTranslationError += translationError(truePose, optimPose);
				optimAvgRotationError += rotationError(truePose, optimPose);
				imageNumber += 1;

				// Save optimization 
				savePose(optimPose, directory.absoluteFilePath(file.completeBaseName() + "_optim.txt"));
				// Render each image and the error maps in a separate folder
				const auto optimImage = ui.viewerWidget->renderToImage(optimPose);
				optimImage.save(directory.absoluteFilePath(file.completeBaseName() + "_optim.png"));
				const auto predImage = ui.viewerWidget->renderToImage(predPose);
				predImage.save(directory.absoluteFilePath(file.completeBaseName() + "_pred.png"));
				const auto errorImage = mseSimilarityErrorMap(optimImage, targetImage);
				errorImage.save(directory.absoluteFilePath(file.completeBaseName() + "_rror.png"));
			}
		}
	}

	predAvgMaxTranslationError /= float(imageNumber);
	predAvgMaxRotationError /= float(imageNumber);
	predAvgTranslationError /= float(imageNumber);
	predAvgRotationError /= float(imageNumber);
	optimAvgMaxTranslationError /= float(imageNumber);
	optimAvgMaxRotationError /= float(imageNumber);
	optimAvgTranslationError /= float(imageNumber);
	optimAvgRotationError /= float(imageNumber);

	qInfo() << "Number of images: " << imageNumber;
	qInfo() << "Average infinity norm for translations: " << predAvgMaxTranslationError << " -> " << optimAvgMaxTranslationError;
	qInfo() << "Average infinity norm for rotations: " << predAvgMaxRotationError << " -> " << optimAvgMaxRotationError;
	qInfo() << "Average distance (translation): " << predAvgTranslationError << " -> " << optimAvgTranslationError;
	qInfo() << "Average distance (rotation): " << predAvgRotationError << " -> " << optimAvgRotationError;
}

void MainWindow::setupUi()
{
	ui.setupUi(this);

	connect(ui.actionRender, &QAction::triggered, this, &MainWindow::render);
}
