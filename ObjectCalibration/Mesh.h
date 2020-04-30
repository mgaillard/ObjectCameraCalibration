#pragma once

#include <vector>

#include <QVector3D>
#include <QImage>

class Mesh
{
public:

	Mesh() = default;

	explicit Mesh(std::vector<QVector3D> vertices,
		          std::vector<QVector3D> uvs,
		          std::vector<unsigned int> indices,
		          QImage texture);

	/**
	 * \brief Clear the mesh
	 */
	void reset();
	
	/**
	 * \brief Load an OBJ mesh using Assimp
	 * \param filename Path to an OBJ file
	 * \return True if loading was successful
	 */
	bool load(const std::string& filename);

	const std::vector<QVector3D>& vertices() const { return m_vertices; }
	const std::vector<QVector3D>& uvs() const { return m_uvs; }
	const std::vector<unsigned int>& indices() const { return m_indices; }
	const QImage& texture() const { return m_texture; }

	/**
	 * \brief Output the vertices and UV coordinates interleaved in a vector
	 * \return The vertices and UV coordinates in a vector
	 */
	std::vector<QVector3D> verticesAndUv() const;

	/**
	 * \brief Create and return a checkerboard pattern mesh
	 * \return A checkerboard pattern mesh
	 */
	static Mesh createCheckerBoardPattern();
	
private:

	std::vector<QVector3D> m_vertices;
	std::vector<QVector3D> m_uvs;

	std::vector<unsigned int> m_indices;

	QImage m_texture;
};

