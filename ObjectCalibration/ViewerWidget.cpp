#include "ViewerWidget.h"

#include <QtMath>
#include <QWheelEvent>

ViewerWidget::ViewerWidget(QWidget* parent) :
	QOpenGLWidget(parent),
	m_logger(new QOpenGLDebugLogger(this)),
	m_camera({ 0.0, 0.0, 1.0 },
		     { 0.0, 0.0, 0.0 },
		     { -1.0, 0.0, 1.0 },
		     qRadiansToDegrees(2.0 * atan(4.29 / (2.0 * 4.5))),
		     4.0f / 3.0f, 0.01f, 10.0f),
	m_objectVbo(QOpenGLBuffer::VertexBuffer),
	m_objectEbo(QOpenGLBuffer::IndexBuffer),
	m_objectTexture(QOpenGLTexture::Target2D)
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
		image.save("framebuffer.png");
	}
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

	// Init Program
	m_program = std::make_unique<QOpenGLShaderProgram>();
	m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, shader_dir + "object_vs.glsl");
	m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, shader_dir + "object_fs.glsl");
	m_program->link();
	m_program->bind();

	// Initialize the frame buffer
	m_frameBuffer = std::make_unique<QOpenGLFramebufferObject>(1008, 756);

	// Initialize vertices
	initializeVbo();
	initializeEbo();
	initializeTexture();

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
	m_objectTexture.setFormat(QOpenGLTexture::RGBA32F);
	m_objectTexture.setMinificationFilter(QOpenGLTexture::Linear);
	m_objectTexture.setMagnificationFilter(QOpenGLTexture::Linear);
	m_objectTexture.setWrapMode(QOpenGLTexture::ClampToEdge);
	m_objectTexture.setSize(textureImage.width(), textureImage.height());
	m_objectTexture.setData(textureImage.mirrored(), QOpenGLTexture::DontGenerateMipMaps);
}
