#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLDebugLogger>

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

#include "Camera.h"
#include "ObjectPose.h"

class ViewerWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core
{
	Q_OBJECT

public:
	explicit ViewerWidget(QWidget* parent = Q_NULLPTR);
	virtual ~ViewerWidget();

	ViewerWidget(const ViewerWidget& widget) = delete;
	ViewerWidget& operator=(ViewerWidget other) = delete;
	ViewerWidget(ViewerWidget&&) = delete;
	ViewerWidget& operator=(ViewerWidget&&) = delete;
	
public slots:
	void cleanup();
	void printInfo();

	void setTargetImage(const QImage& targetImage);
	
	void moveCamera(const ObjectPose& pose);
	
	QImage renderToImage(const ObjectPose& pose);

	float renderAndComputeSimilarityCpu(const ObjectPose& pose);
	float renderAndComputeSimilarityGpu(const ObjectPose& pose);

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:

	void initialize();
	void initializeVbo();
	void initializeEbo();
	void initializeTexture();
	void initializeTargetTexture();
	void initializeComputeShader();

	void render(const ObjectPose& pose);
	
	QOpenGLDebugLogger* m_logger;

	OrbitCamera m_camera;

	std::unique_ptr<QOpenGLShaderProgram> m_program;
	std::unique_ptr<QOpenGLShaderProgram> m_computeSimilarityProgram;

	// Add sheet of paper with UV coordinates
	QOpenGLVertexArrayObject m_objectVao;
	QOpenGLBuffer m_objectVbo;
	QOpenGLBuffer m_objectEbo;
	QOpenGLTexture m_objectTexture;

	// Texture in which to render
	std::unique_ptr<QOpenGLFramebufferObject> m_frameBuffer;

	// Atomic buffer for computing the similarity in the compute shader
	GLuint m_similarityAtomicBuffer;
	
	// Target texture
	QImage m_targetImage;
	QOpenGLTexture m_targetTexture;

	// TODO: Add a compute shader to find the distance between two images
};
