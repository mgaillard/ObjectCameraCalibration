#include "Mesh.h"

#include <QDebug>
#include <QFileInfo>
#include <QDir>

#include "OBJ_Loader.h"

Mesh::Mesh(std::vector<QVector3D> vertices,
	       std::vector<QVector3D> uvs,
	       std::vector<unsigned int> indices,
	       QImage texture) :
	m_vertices(std::move(vertices)),
	m_uvs(std::move(uvs)),
	m_indices(std::move(indices)),
	m_texture(std::move(texture))
{
	
}

void Mesh::reset()
{
	m_vertices.clear();
	m_uvs.clear();
	m_indices.clear();
	m_texture = QImage();
}

bool Mesh::load(const std::string& filename)
{
	// Initialize Loader
	objl::Loader loader;

	const bool isLoaded = loader.LoadFile(filename);
	
	if (!isLoaded)
	{
		qWarning() << "Could not load the OBJ file " << QString::fromStdString(filename);
		return false;
	}

	// Only load the first mesh
	auto currentMesh = loader.LoadedMeshes.front();

	reset();
	
	for (const auto& vertex : currentMesh.Vertices)
	{
		m_vertices.emplace_back(
			vertex.Position.X,
			vertex.Position.Y,
			vertex.Position.Z
		);

		m_uvs.emplace_back(
			vertex.TextureCoordinate.X,
			vertex.TextureCoordinate.Y,
			0.0
		);
	}

	for (unsigned int index : currentMesh.Indices)
	{
		m_indices.push_back(index);
	}

	// Load the texture
	const auto textureFile = QString::fromStdString(currentMesh.MeshMaterial.map_Ka);
	const QFileInfo objFileInfo(QString::fromStdString(filename));
	const auto directory = objFileInfo.dir();

	if (directory.exists(textureFile))
	{
		m_texture = QImage(directory.filePath(textureFile));
	}

	return true;
}

std::vector<QVector3D> Mesh::verticesAndUv() const
{
	std::vector<QVector3D> data;

	data.reserve(m_vertices.size() + m_uvs.size());

	auto itVertices = m_vertices.cbegin();
	auto itUvs = m_uvs.cbegin();
	
	while (itVertices != m_vertices.cend() || itUvs != m_uvs.cend())
	{
		if (itVertices != m_vertices.cend())
		{
			data.push_back(*itVertices);
			++itVertices;
		}

		if (itUvs != m_uvs.cend())
		{
			data.push_back(*itUvs);
			++itUvs;
		}
	}
	
	return data;
}

Mesh Mesh::createCheckerBoardPattern()
{
	const std::vector<QVector3D> vertices = {
		{-0.1485, -0.105, 0.0},
		{0.1485, -0.105, 0.0},
		{0.1485, 0.105, 0.0},
		{-0.1485, 0.105, 0.0}
	};
	
	const std::vector<QVector3D> uvs = {
		{ 0.0, 0.0, 0.0 },
		{ 1.0, 0.0, 0.0 },
		{ 1.0, 1.0, 0.0 },
		{ 0.0, 1.0, 0.0 }
	};

	const std::vector<unsigned int> indices = { 0, 1, 3, 1, 2, 3 };

	const QImage texture(":/MainWindow/Resources/pattern.png");
	
	return Mesh(vertices, uvs, indices, texture);
}
