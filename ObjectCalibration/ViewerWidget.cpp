#include "ViewerWidget.h"

#include <QtMath>
#include <QWheelEvent>

#include "Similarity.h"

ViewerWidget::ViewerWidget(QWidget* parent) :
	QOpenGLWidget(parent),
	m_logger(new QOpenGLDebugLogger(this)),
	m_camera({ 0.0, 0.0, 1.0 },
		     { 0.0, 0.0, 0.0 },
		     { -1.0, 0.0, 0.0 },
		     qRadiansToDegrees(2.0 * atan(4.29 / (2.0 * 4.5))),
		     4.0f / 3.0f, 0.01f, 10.0f),
	m_objectVbo(QOpenGLBuffer::VertexBuffer),
	m_objectEbo(QOpenGLBuffer::IndexBuffer),
	m_objectTexture(QOpenGLTexture::Target2D),
	m_similarityAtomicBuffer(0),
	m_targetTexture(QOpenGLTexture::Target2D)
{
	m_objectMatrix.setToIdentity();

	// Pattern
	// m_object = Mesh::createCheckerBoardPattern();

	// Cat
	// m_object.load("../blender/objects/cat/centered_cat.obj");
	// m_objectMatrix.scale(0.006);

	// Phone
	m_object.load("../blender/objects/phone/phone.obj");
	m_objectMatrix.translate(0.00155178, -0.0191204, -0.0446233);
	m_objectMatrix.scale(0.01);
}

ViewerWidget::~ViewerWidget()
{
	cleanup();
}

void ViewerWidget::cleanup()
{
	if (m_program)
	{
		m_objectVao.destroy();
		m_objectVbo.destroy();
		m_objectVbo.destroy();
		m_objectTexture.destroy();
		m_program.reset(nullptr);
		m_frameBuffer.reset(nullptr);
	}
}

void ViewerWidget::printInfo()
{
	qDebug() << "Vendor: " << QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	qDebug() << "Renderer: " << QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	qDebug() << "Version: " << QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	qDebug() << "GLSL Version: " << QString::fromLatin1(reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
}

void ViewerWidget::moveCamera(const ObjectPose& pose)
{
	// Compute coordinates of the camera in spherical frame
	const auto rotation = QQuaternion::fromEulerAngles(pose.rotation.x(), pose.rotation.y(), pose.rotation.z());
	const auto rotation3x3 = rotation.toRotationMatrix();
	const QMatrix4x4 rotation4x4(rotation3x3);

	// Translation of the camera
	QMatrix4x4 translation;
	translation.translate(pose.translation);

	// Complete transformation of the camera
	const auto transformation = translation * rotation4x4;
	const QMatrix4x4 normalMatrix(transformation.normalMatrix());

	// Position of camera before transformation
	const QVector3D originalCameraEye(0.0, 0.0, 1.0);
	const QVector3D originalCameraUp(-1.0, 0.0, 0.0);
	const QVector3D originalCameraAt(0.0, 0.0, 0.0);

	// Transform the camera
	m_camera.setEye(transformation.map(originalCameraEye));
	m_camera.setUp(normalMatrix.map(originalCameraUp));
	m_camera.setAt(translation.map(originalCameraAt));
}

void ViewerWidget::moveObject(const ObjectPose& pose)
{
	QMatrix4x4 translationMatrix;
	translationMatrix.translate(pose.translation);

	const auto rotation = QQuaternion::fromEulerAngles(pose.rotation.x(), pose.rotation.y(), pose.rotation.z());
	const QMatrix4x4 rotationMatrix(rotation.toRotationMatrix());
	
	m_objectWorldMatrix = translationMatrix * rotationMatrix * m_objectMatrix;
}

void ViewerWidget::setTargetImage(const QImage& targetImage)
{
	m_targetImage = targetImage;

	makeCurrent();
	initializeTargetTexture();
	doneCurrent();
}

void ViewerWidget::render(const ObjectPose& pose)
{
	auto f = context()->versionFunctions<QOpenGLFunctions_4_3_Core>();

	moveObject(pose);
	
	if (m_frameBuffer)
	{
		// Attach the frame buffer and set the resolution of the viewport
		m_frameBuffer->bind();
		glViewport(0, 0, m_frameBuffer->width(), m_frameBuffer->height());
		m_camera.setAspectRatio(float(m_frameBuffer->width()) / float(m_frameBuffer->height()));
		m_camera.setEye({ 0.0, 0.0, 1.0 });
		m_camera.setAt({ 0.0, 0.0, 0.0 });
		m_camera.setUp({ -1.0, 0.0, 0.0 });
		m_camera.setFovy(qRadiansToDegrees(2.0 * atan(4.29 / (2.0 * 4.5))));
		m_camera.setNearPlane(0.01f);
		m_camera.setFarPlane(10.0f);
	}

	// Transparent background
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// Enable transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Paint the object in the frame buffer
	if (m_program)
	{
		// Setup matrices
		const auto normalMatrix = m_objectWorldMatrix.normalMatrix();
		const auto viewMatrix = m_camera.viewMatrix();
		const auto projectionMatrix = m_camera.projectionMatrix();
		const auto pvMatrix = projectionMatrix * viewMatrix;
		const auto pvmMatrix = pvMatrix * m_objectWorldMatrix;

		m_program->bind();

		// Update matrices
		m_program->setUniformValue("P", projectionMatrix);
		m_program->setUniformValue("V", viewMatrix);
		m_program->setUniformValue("M", m_objectWorldMatrix);
		m_program->setUniformValue("N", normalMatrix);
		m_program->setUniformValue("PV", pvMatrix);
		m_program->setUniformValue("PVM", pvmMatrix);

		// Bind the VAO containing the patches
		QOpenGLVertexArrayObject::Binder vaoBinder(&m_objectVao);

		// Bind the texture
		const auto textureUnit = 0;
		m_program->setUniformValue("image", textureUnit);
		m_objectTexture.bind(textureUnit);

		f->glDrawElements(GL_TRIANGLES,
			m_objectEbo.size(),
			GL_UNSIGNED_INT,
			nullptr);

		m_program->release();
	}

	if (m_frameBuffer)
	{
		m_frameBuffer->release();
	}
}

QImage ViewerWidget::renderToImage(const ObjectPose& pose)
{
	makeCurrent();

	// Output the content of the frame buffer
	render(pose);
	const auto result = m_frameBuffer->toImage();

	doneCurrent();

	return result;
}

float ViewerWidget::renderAndComputeSimilarityCpu(const ObjectPose& pose)
{
	const auto image = renderToImage(pose);

	return computeSimilarity(image, m_targetImage);
}

float ViewerWidget::renderAndComputeSimilarityGpu(const ObjectPose& pose)
{
	float similarity = 0.0f;
	
	makeCurrent();

	// Render the image in the frame buffer
	render(pose);

	auto f = context()->versionFunctions<QOpenGLFunctions_4_3_Core>();
	
	// Use the compute shader
	if (m_computeSimilarityProgram)
	{
		// Local size in the compute shader
		const int localSizeX = 4;
		const int localSizeY = 4;
		const float multiplicationBeforeRound = 100.0;

		m_computeSimilarityProgram->bind();

		// Coefficient when multiplying before rounding (the number of decimals we keep)
		m_computeSimilarityProgram->setUniformValue("multiplication_before_round", multiplicationBeforeRound);

		// Bind the target texture as an image
		const auto targetImageUnit = 0;
		f->glBindImageTexture(targetImageUnit, m_targetTexture.textureId(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

		// Bind the frame buffer texture as an image
		const auto frameBufferImageUnit = 1;
		f->glBindImageTexture(frameBufferImageUnit, m_frameBuffer->texture(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

		// Bind the atomic counters (binding = 2)
		f->glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_similarityAtomicBuffer);
		f->glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_similarityAtomicBuffer);
		// Init the 6 atomic counters with a value of 0
		GLuint counterValues[6] = { 0, 0, 0, 0, 0, 0 };
		f->glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, 6 * sizeof(GLuint), counterValues);

		// Compute the number of blocks in each dimensions
		const int blocksX = std::max(1, 1 + ((m_frameBuffer->width() - 1) / localSizeX));
		const int blocksY = std::max(1, 1 + ((m_frameBuffer->height() - 1) / localSizeY));
		// Launch the compute shader and wait for it to finish
		f->glDispatchCompute(blocksX, blocksY, 1);
		f->glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);

		// Read back values of the atomic counters
		f->glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, 6 * sizeof(GLuint), counterValues);

		// Unbind atomic buffer and textures
		f->glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		f->glBindImageTexture(frameBufferImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		f->glBindImageTexture(targetImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

		m_computeSimilarityProgram->release();

		const auto tp = float(counterValues[0]);
		const auto fp = float(counterValues[1]);
		const auto fn = float(counterValues[2]);
		const auto numerator = float(counterValues[3]) / multiplicationBeforeRound;
		const auto denominator = float(counterValues[4]) / multiplicationBeforeRound;
		const auto maeSum = float(counterValues[5]);

		// Compute the Dice Coefficient
		const auto diceCoefficient = (2.f * tp) / (2.f * tp + fp + fn);

		// Compute the fuzzy Dice Coefficient
		const auto fuzzyDiceCoefficient = (2.f * numerator + 1.f) / (denominator + 1.f);

		// Compute the mean absolute error
		float mae = 0.0;
		if (tp > 0.0)
		{
			mae = maeSum / float(tp);
		}

		// The similarity is 90% based on silhouettes overlap and 10% based on absolute error
		similarity = 0.9f * fuzzyDiceCoefficient + 0.1f * mae;
	}
	doneCurrent();

	return similarity;
}

void ViewerWidget::initializeGL()
{
	connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ViewerWidget::cleanup);

	initializeOpenGLFunctions();
	m_logger->initialize();

	// Print OpenGL info and Debug messages
	printInfo();

	// Init the scene
	initialize();
}

void ViewerWidget::resizeGL(int w, int h)
{
	m_camera.setAspectRatio(float(w) / h);
}

void ViewerWidget::paintGL()
{
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ViewerWidget::mousePressEvent(QMouseEvent* event)
{
	const auto x = event->globalX();
	const auto y = event->globalY();

	if (event->button() == Qt::LeftButton)
	{
		m_camera.mouseLeftButtonPressed(x, y);
	}
	else if (event->button() == Qt::RightButton)
	{
		m_camera.mouseRightButtonPressed(x, y);
	}

	update();
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
	m_camera.mouseReleased();

	update();
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
	const auto x = event->globalX();
	const auto y = event->globalY();

	m_camera.mouseMoved(x, y);

	update();
}

void ViewerWidget::wheelEvent(QWheelEvent* event)
{
	// Default speed
	float speed = 1.0;

	if (event->modifiers() & Qt::ShiftModifier)
	{
		// If shift is used, zooming is 4 times faster
		speed = 4.0;
	}
	else if (event->modifiers() & Qt::ControlModifier)
	{
		// If control is used, zooming is four times slower
		speed = 0.25;
	}

	const auto numDegrees = event->angleDelta() / 8;
	const auto numSteps = numDegrees / 15;
	m_camera.zoom(speed * numSteps.y());

	update();
}

void ViewerWidget::initialize()
{
	const QString shader_dir = ":/MainWindow/Shaders/";

	// Init shaders
	m_program = std::make_unique<QOpenGLShaderProgram>();
	m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, shader_dir + "object_vs.glsl");
	m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, shader_dir + "object_fs.glsl");
	m_program->link();
	m_program->bind();

	// Initialize the frame buffer with a depth buffer
	QOpenGLFramebufferObjectFormat fboFormat;
	fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
	m_frameBuffer = std::make_unique<QOpenGLFramebufferObject>(QSize(4032, 3024), fboFormat);

	// Initialize vertices
	initializeVbo();
	initializeEbo();
	initializeTexture();
	initializeTargetTexture();
	initializeComputeShader();

	// Init VAO
	m_objectVao.create();
	QOpenGLVertexArrayObject::Binder vaoBinder(&m_objectVao);

	// Configure VBO
	m_objectVbo.bind();
	const auto posLoc = 0;
	const auto uvLoc = 1;
	m_program->enableAttributeArray(posLoc);
	m_program->enableAttributeArray(uvLoc);
	m_program->setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 2 * sizeof(QVector3D));
	m_program->setAttributeBuffer(uvLoc, GL_FLOAT, sizeof(QVector3D), 3, 2 * sizeof(QVector3D));

	// Configure EBO
	m_objectEbo.bind();

	m_program->release();
}

void ViewerWidget::initializeVbo()
{	
	std::vector<QVector3D> data = m_object.verticesAndUv();

	// Init VBO
	m_objectVbo.create();
	m_objectVbo.bind();
	m_objectVbo.allocate(data.data(), data.size() * sizeof(QVector3D));
	m_objectVbo.release();
}

void ViewerWidget::initializeEbo()
{
	// Indices of faces
	std::vector<GLuint> indices = m_object.indices();

	// Init VBO
	m_objectEbo.create();
	m_objectEbo.bind();
	m_objectEbo.allocate(indices.data(), indices.size() * sizeof(GLuint));
	m_objectEbo.release();
}


void ViewerWidget::initializeTexture()
{
	const QImage textureImage = m_object.texture();
	
	m_objectTexture.destroy();
	m_objectTexture.create();
	m_objectTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
	m_objectTexture.setMinificationFilter(QOpenGLTexture::Linear);
	m_objectTexture.setMagnificationFilter(QOpenGLTexture::Linear);
	m_objectTexture.setWrapMode(QOpenGLTexture::ClampToEdge);
	m_objectTexture.setSize(textureImage.width(), textureImage.height());
	m_objectTexture.setData(textureImage.mirrored(), QOpenGLTexture::DontGenerateMipMaps);
}

void ViewerWidget::initializeTargetTexture()
{
	if (m_targetImage.width() > 0 && m_targetImage.height() > 0)
	{
		m_targetTexture.destroy();
		m_targetTexture.create();
		m_targetTexture.setFormat(QOpenGLTexture::RGBA8_UNorm);
		m_targetTexture.setMinificationFilter(QOpenGLTexture::Linear);
		m_targetTexture.setMagnificationFilter(QOpenGLTexture::Linear);
		m_targetTexture.setWrapMode(QOpenGLTexture::ClampToEdge);
		m_targetTexture.setSize(m_targetImage.width(), m_targetImage.height());
		m_targetTexture.setData(m_targetImage.mirrored(), QOpenGLTexture::DontGenerateMipMaps);
	}
}

void ViewerWidget::initializeComputeShader()
{
	const QString shader_dir = ":/MainWindow/Shaders/";
	
	// Init similarity compute shader
	m_computeSimilarityProgram = std::make_unique<QOpenGLShaderProgram>();
	m_computeSimilarityProgram->addShaderFromSourceFile(QOpenGLShader::Compute, shader_dir + "similarity_cs.glsl");
	m_computeSimilarityProgram->link();

	auto f = context()->versionFunctions<QOpenGLFunctions_4_3_Core>();

	// Declare and generate a buffer object name
	f->glGenBuffers(1, &m_similarityAtomicBuffer);
	// Bind the buffer and define its initial storage capacity (6 uint)
	f->glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_similarityAtomicBuffer);
	f->glBufferData(GL_ATOMIC_COUNTER_BUFFER, 6 * sizeof(GLuint), NULL, GL_DYNAMIC_READ);
	// Unbind the buffer 
	f->glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}
