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
	m_targetTexture(QOpenGLTexture::Target2D)
{
	
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

void ViewerWidget::moveCamera(float xAngle, float yAngle, float zAngle)
{
	// Compute coordinates of the camera in spherical frame
	const auto rotation = QQuaternion::fromEulerAngles(xAngle, yAngle, zAngle);
	const auto rotation3x3 = rotation.toRotationMatrix();
	const QMatrix4x4 rotation4x4(rotation3x3);
	const QMatrix4x4 rotationNormalMatrix4x4(rotation4x4.normalMatrix());

	const QVector3D originalCameraPosition(0.0, 0.0, 1.0);
	const QVector3D originalCameraUp(-1.0, 0.0, 0.0);

	// Rotate the camera
	m_camera.setEye(rotation4x4.map(originalCameraPosition));
	m_camera.setUp(rotationNormalMatrix4x4.map(originalCameraUp));
}

void ViewerWidget::setTargetImage(const QImage& targetImage)
{
	m_targetImage = targetImage;

	makeCurrent();
	initializeTargetTexture();
	doneCurrent();
}

void ViewerWidget::render(float xAngle, float yAngle, float zAngle)
{
	auto f = context()->versionFunctions<QOpenGLFunctions_4_3_Core>();

	moveCamera(xAngle, yAngle, zAngle);
	
	if (m_frameBuffer)
	{
		// Attach the frame buffer and set the resolution of the viewport
		m_frameBuffer->bind();
		glViewport(0, 0, m_frameBuffer->width(), m_frameBuffer->height());
		m_camera.setAspectRatio(float(m_frameBuffer->width()) / float(m_frameBuffer->height()));
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
		const QMatrix4x4 worldMatrix;
		const auto normalMatrix = worldMatrix.normalMatrix();
		const auto viewMatrix = m_camera.viewMatrix();
		const auto projectionMatrix = m_camera.projectionMatrix();
		const auto pvMatrix = projectionMatrix * viewMatrix;
		const auto pvmMatrix = pvMatrix * worldMatrix;

		m_program->bind();

		// Update matrices
		m_program->setUniformValue("P", projectionMatrix);
		m_program->setUniformValue("V", viewMatrix);
		m_program->setUniformValue("M", worldMatrix);
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

QImage ViewerWidget::renderToImage(float xAngle, float yAngle, float zAngle)
{
	makeCurrent();

	// Output the content of the frame buffer
	render(xAngle, yAngle, zAngle);
	const auto result = m_frameBuffer->toImage();

	doneCurrent();

	return result;
}

float ViewerWidget::renderAndComputeSimilarityCpu(float xAngle, float yAngle, float zAngle)
{
	const auto image = renderToImage(xAngle, yAngle, zAngle);

	return computeSimilarity(image, m_targetImage);
}

float ViewerWidget::renderAndComputeSimilarityGpu(float xAngle, float yAngle, float zAngle)
{
	float diceCoefficient = 0.0f;
	
	makeCurrent();

	// Render the image in the frame buffer
	render(xAngle, yAngle, zAngle);

	auto f = context()->versionFunctions<QOpenGLFunctions_4_3_Core>();
	
	// Use the compute shader
	if (m_computeSimilarityProgram)
	{
		// Local size in the compute shader
		const int localSizeX = 4;
		const int localSizeY = 4;

		m_computeSimilarityProgram->bind();

		// Bind the target texture as an image
		const auto targetImageUnit = 0;
		f->glBindImageTexture(targetImageUnit, m_targetTexture.textureId(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

		// Bind the frame buffer texture as an image
		const auto frameBufferImageUnit = 1;
		f->glBindImageTexture(frameBufferImageUnit, m_frameBuffer->texture(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

		// Bind the atomic counters (binding = 2)
		f->glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_similarityAtomicBuffer);
		f->glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_similarityAtomicBuffer);
		// Init the 3 atomic counters with a value of 0
		GLuint counterValues[3] = { 0, 0, 0 };
		f->glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, 3 * sizeof(GLuint), counterValues);

		// Compute the number of blocks in each dimensions
		const int blocksX = std::max(1, 1 + ((m_frameBuffer->width() - 1) / localSizeX));
		const int blocksY = std::max(1, 1 + ((m_frameBuffer->height() - 1) / localSizeY));
		// Launch the compute shader and wait for it to finish
		f->glDispatchCompute(blocksX, blocksY, 1);
		f->glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);

		// Read back values of the atomic counters
		f->glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, 3 * sizeof(GLuint), counterValues);

		// Unbind atomic buffer and textures
		f->glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		f->glBindImageTexture(frameBufferImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		f->glBindImageTexture(targetImageUnit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

		m_computeSimilarityProgram->release();

		const auto tp = float(counterValues[0]);
		const auto fp = float(counterValues[1]);
		const auto fn = float(counterValues[2]);

		diceCoefficient = (2.f * tp) / (2.f * tp + fp + fn);
	}
	doneCurrent();

	return diceCoefficient;
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
	
	/*
	auto f = context()->versionFunctions<QOpenGLFunctions_4_3_Core>();
	
	if (m_frameBuffer)
	{
		// Attach the frame buffer and set the resolution of the viewport
		m_frameBuffer->bind();
		glViewport(0, 0, m_frameBuffer->width(), m_frameBuffer->height());
		m_camera.setAspectRatio(float(m_frameBuffer->width()) / m_frameBuffer->height());
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
		const QMatrix4x4 worldMatrix;
		const auto normalMatrix = worldMatrix.normalMatrix();
		const auto viewMatrix = m_camera.viewMatrix();
		const auto projectionMatrix = m_camera.projectionMatrix();
		const auto pvMatrix = projectionMatrix * viewMatrix;
		const auto pvmMatrix = pvMatrix * worldMatrix;

		m_program->bind();

		// Update matrices
		m_program->setUniformValue("P", projectionMatrix);
		m_program->setUniformValue("V", viewMatrix);
		m_program->setUniformValue("M", worldMatrix);
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

		// Save content of frame buffer
		const QImage image(m_frameBuffer->toImage());
		// Compare to the target image
		const QImage targetImage(":/MainWindow/Resources/target.png");

		// image.save("image.png");
		
		qDebug() << computeSimilarity(image, targetImage);
	}
	*/
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

	// Initialize the frame buffer
	m_frameBuffer = std::make_unique<QOpenGLFramebufferObject>(4032, 3024);

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
	std::vector<QVector3D> data = {
		{-0.105, -0.1485, 0.0}, // Vertex
		{0.0, 1.0, 0.0},		 // UV
		{0.105, -0.1485, 0.0},  // Vertex
		{0.0, 0.0, 0.0},		 // UV
		{0.105, 0.1485, 0.0},   // Vertex
		{1.0, 0.0, 0.0},		 // UV
		{-0.105, 0.1485, 0.0},  // Vertex
		{1.0, 1.0, 0.0},		 // UV
	};

	// Init VBO
	m_objectVbo.create();
	m_objectVbo.bind();
	m_objectVbo.allocate(data.data(), data.size() * sizeof(QVector3D));
	m_objectVbo.release();
}

void ViewerWidget::initializeEbo()
{
	// Indices of faces
	std::vector<GLuint> indices = {0, 1, 3, 1, 2, 3};

	// Init VBO
	m_objectEbo.create();
	m_objectEbo.bind();
	m_objectEbo.allocate(indices.data(), indices.size() * sizeof(GLuint));
	m_objectEbo.release();
}


void ViewerWidget::initializeTexture()
{
	const QImage textureImage(":/MainWindow/Resources/pattern.png");
	
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
	// Bind the buffer and define its initial storage capacity (3 uint)
	f->glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_similarityAtomicBuffer);
	f->glBufferData(GL_ATOMIC_COUNTER_BUFFER, 3 * sizeof(GLuint), NULL, GL_DYNAMIC_READ);
	// Unbind the buffer 
	f->glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}
