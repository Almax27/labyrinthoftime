#include "pch.h"
#include "Terrain.h"
#include <iostream>


Terrain::Terrain()
{
	m_terrainGeneratedToggle = false;
}


Terrain::~Terrain()
{
}

bool Terrain::Initialize(ID3D11Device* device, int terrainWidth, int terrainHeight)
{
	int index;
	float height = 0.0;
	bool result;
	
	// Save the dimensions of the terrain.
	m_terrainWidth = terrainWidth;
	m_terrainHeight = terrainHeight;

	m_frequency = m_terrainWidth / 20;
	m_amplitude = 3.0;
	m_wavelength = 1;

	// Create the structure to hold the terrain data.
	m_heightMap = new HeightMapType[m_terrainWidth * m_terrainHeight];
	if (!m_heightMap)
	{
		return false;
	}

	//this is how we calculate the texture coordinates first calculate the step size there will be between vertices. 
	// 5.0f tile 5 times across the terrain.
	// tile 1.0f across terrain 512x512
	float textureCoordinatesStep = 1.0f / m_terrainWidth;  

	// Initialise the data in the height map (flat).
	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)height;
			m_heightMap[index].z = (float)j;

			//and use this step to calculate the texture coordinates for this point on the terrain.
			m_heightMap[index].u = (float)i * textureCoordinatesStep;
			m_heightMap[index].v = (float)j * textureCoordinatesStep;

		}
	}

	//even though we are generating a flat terrain, we still need to normalise it. 
	// Calculate the normals for the terrain data.
	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	// Initialize the vertex and index buffer that hold the geometry for the terrain. // second step - transfers array info into vertex buffers for DX Rendering
	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}

	
	return true;
}

void Terrain::Render(ID3D11DeviceContext * deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);
	deviceContext->DrawIndexed(m_indexCount, 0, 0);

	return;
}

bool Terrain::CalculateNormals()
{
	int i, j, index1, index2, index3, index, count;
	float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
	DirectX::SimpleMath::Vector3* normals;
	

	// Create a temporary array to hold the un-normalized normal vectors.
	normals = new DirectX::SimpleMath::Vector3[(m_terrainHeight - 1) * (m_terrainWidth - 1)];
	if (!normals)
	{
		return false;
	}

	// Go through all the faces in the mesh and calculate their normals.
	for (j = 0; j<(m_terrainHeight - 1); j++)
	{
		for (i = 0; i<(m_terrainWidth - 1); i++)
		{
			index1 = (j * m_terrainHeight) + i;
			index2 = (j * m_terrainHeight) + (i + 1);
			index3 = ((j + 1) * m_terrainHeight) + i;

			// Get three vertices from the face.
			vertex1[0] = m_heightMap[index1].x;
			vertex1[1] = m_heightMap[index1].y;
			vertex1[2] = m_heightMap[index1].z;

			vertex2[0] = m_heightMap[index2].x;
			vertex2[1] = m_heightMap[index2].y;
			vertex2[2] = m_heightMap[index2].z;

			vertex3[0] = m_heightMap[index3].x;
			vertex3[1] = m_heightMap[index3].y;
			vertex3[2] = m_heightMap[index3].z;

			// Calculate the two vectors for this face.
			vector1[0] = vertex1[0] - vertex3[0];
			vector1[1] = vertex1[1] - vertex3[1];
			vector1[2] = vertex1[2] - vertex3[2];
			vector2[0] = vertex3[0] - vertex2[0];
			vector2[1] = vertex3[1] - vertex2[1];
			vector2[2] = vertex3[2] - vertex2[2];

			index = (j * (m_terrainHeight - 1)) + i;

			// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
			normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
		}
	}

	// Now go through all the vertices and take an average of each face normal 	
	// that the vertex touches to get the averaged normal for that vertex.
	for (j = 0; j<m_terrainHeight; j++)
	{
		for (i = 0; i<m_terrainWidth; i++)
		{
			// Initialize the sum.
			sum[0] = 0.0f;
			sum[1] = 0.0f;
			sum[2] = 0.0f;

			// Initialize the count.
			count = 0;

			// Bottom left face.
			if (((i - 1) >= 0) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Bottom right face.
			if ((i < (m_terrainWidth - 1)) && ((j - 1) >= 0))
			{
				index = ((j - 1) * (m_terrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper left face.
			if (((i - 1) >= 0) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainHeight - 1)) + (i - 1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper right face.
			if ((i < (m_terrainWidth - 1)) && (j < (m_terrainHeight - 1)))
			{
				index = (j * (m_terrainHeight - 1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Take the average of the faces touching this vertex.
			sum[0] = (sum[0] / (float)count);
			sum[1] = (sum[1] / (float)count);
			sum[2] = (sum[2] / (float)count);

			// Calculate the length of this normal.
			length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));

			// Get an index to the vertex location in the height map array.
			index = (j * m_terrainHeight) + i;

			// Normalize the final shared normal for this vertex and store it in the height map array.
			m_heightMap[index].nx = (sum[0] / length);
			m_heightMap[index].ny = (sum[1] / length);
			m_heightMap[index].nz = (sum[2] / length);
		}
	}

	// Release the temporary normals.
	delete[] normals;
	normals = 0;

	return true;
}

DirectX::SimpleMath::Vector3 Terrain::GetDimensions()
{
	DirectX::SimpleMath::Vector3  m_terrainDimensions;
	m_terrainDimensions.x = m_terrainWidth;
	m_terrainDimensions.z = m_terrainHeight;
	return m_terrainDimensions;
}

void Terrain::Shutdown()
{
	// Release the index buffer.
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

bool Terrain::InitializeBuffers(ID3D11Device * device )
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int index, i, j;
	int index1, index2, index3, index4; //geometric indices. 

	// Calculate the number of vertices in the terrain mesh. // two squares 
	m_vertexCount = (m_terrainWidth - 1) * (m_terrainHeight - 1) * 6;

	// Set the index count to the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if (!indices)
	{
		return false;
	}

	// Initialize the index to the vertex buffer.
	index = 0;
	bool inverted = false;

	for (j = 0; j<(m_terrainHeight - 1); j++)
	{
		for (i = 0; i<(m_terrainWidth - 1); i++)
		{
			
			if (i == 0) {
				if (j % 2 == 0) {
					inverted = false;
				}
				else {
					inverted = true;
				}
			}

			if (inverted) {
				index1 = (m_terrainHeight * j) + (i + 1);      // Bottom right.
				index2 = (m_terrainHeight * (j + 1)) + (i + 1);  // Upper right.
				index3 = (m_terrainHeight * j) + i;          // Bottom left.
				index4 = (m_terrainHeight * (j + 1)) + i;      // Upper left.
			}
			else {
				index1 = (m_terrainHeight * j) + i;          // Bottom left.
				index2 = (m_terrainHeight * j) + (i + 1);      // Bottom right.
				index3 = (m_terrainHeight * (j + 1)) + i;      // Upper left.
				index4 = (m_terrainHeight * (j + 1)) + (i + 1);  // Upper right.
			}
			inverted = !inverted;
			
			// Upper left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index3].nx, m_heightMap[index3].ny, m_heightMap[index3].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index3].u, m_heightMap[index3].v);
			indices[index] = index;
			index++;

			// Upper right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index1].u, m_heightMap[index1].v);
			indices[index] = index;
			index++;

			// Upper right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index4].u, m_heightMap[index4].v);
			indices[index] = index;
			index++;

			// Bottom right.
			vertices[index].position = DirectX::SimpleMath::Vector3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			vertices[index].normal = DirectX::SimpleMath::Vector3(m_heightMap[index2].nx, m_heightMap[index2].ny, m_heightMap[index2].nz);
			vertices[index].texture = DirectX::SimpleMath::Vector2(m_heightMap[index2].u, m_heightMap[index2].v);
			indices[index] = index;
			index++;
		}
	}

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;

	return true;
}

void Terrain::RenderBuffers(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

bool Terrain::GenerateHeightMap(ID3D11Device* device)
{
	bool result;

	int index;
	float height = 0.0;

	m_frequency = (6.283/m_terrainHeight) / m_wavelength; //we want a wavelength of 1 to be a single wave over the whole terrain.  A single wave is 2 pi which is about 6.283
	// m_terrainHeight is actually the z axis

	//loop through the terrain and set the hieghts how we want. This is where we generate the terrain
	//in this case I will run a sin-wave through the terrain in one axis.

	for (int j = 0; j<m_terrainHeight; j++)
	{
		for (int i = 0; i<m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = GenerateRandomNumber(0.5f) * m_amplitude *2.f;
			m_heightMap[index].z = (float)j;
		}
	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}
}

bool Terrain::SmoothenHeightMap(ID3D11Device* device)
{
	bool result;
	
	int index, nx, nz;
	float height = 0.0;
	int neighbours[8] = {}; // array starts at 0, inclusive
	int n = 8;
	
	/* Initialise corner of height map */ // 1.
	int bottomLeftCorner = 0;
	int bottomRightCorner = (m_terrainHeight * (m_terrainHeight - 1));
	int topLeftCorner = (m_terrainWidth - 1);
	int topRightCorner = m_terrainHeight * (m_terrainHeight - 1) + (m_terrainWidth - 1);

	//we want a wavelength of 1 to be a single wave over the whole terrain.  A single wave is 2 pi which is about 6.283
	m_frequency = (6.283 / m_terrainHeight) / m_wavelength; 
	// m_terrainHeight is actually the z axis

	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i; 

			float sum = m_heightMap[index].y;

			// with more than 128 square dimensions, initial neighbours on bottom row might not exist
			// can refractor this better if it works
			if (m_heightMap[index].z + 1 >= m_terrainHeight || m_heightMap[index].z <= 0) {
				// if no neighbours, set height to that of current index/element
				neighbours[0], neighbours [1], neighbours[2] = m_heightMap[index].y;
				}
			else {
				neighbours[0] = m_heightMap[(m_terrainHeight * (j + 1)) + (i - 1)].y; // top left
				neighbours[1] = m_heightMap[(m_terrainHeight * (j + 1)) + (i)].y;     // top middle
				neighbours[2] = m_heightMap[(m_terrainHeight * (j + 1)) + (i + 1)].y; // top right
			}
			neighbours[3] = m_heightMap[(m_terrainHeight * (j)) + (i - 1)].y;	  // middle left
			neighbours[4] = m_heightMap[(m_terrainHeight * (j)) + (i + 1)].y;	  // middle right
			if (m_heightMap[index].z <= 0 || m_heightMap[index].z + 1 >= m_terrainHeight) {
				// if no neighbours, set height to that of current index/element
				neighbours[5], neighbours[6], neighbours[7] = m_heightMap[index].y;
			}
			else {
				neighbours[5] = m_heightMap[(m_terrainHeight * (j - 1)) + (i - 1)].y; // bottom left
				neighbours[6] = m_heightMap[(m_terrainHeight * (j - 1)) + (i)].y;	  // bottom middle
				neighbours[7] = m_heightMap[(m_terrainHeight * (j - 1)) + (i + 1)].y; // bottom right
			}
				

			for (int z = 0; z < n; z++)
			{
				// if out of map, take y of current index for sum
				if (neighbours[z] < 0 || neighbours[z] >= m_terrainHeight * m_terrainWidth) 
				{
					sum += m_heightMap[index].y;
				}
				else
				{
					sum += neighbours[z]; // if exists, include in sum
				}
			}
						
			// smoothen based on neighbours
			m_heightMap[index].y = sum / 9.0f; // current point n is no. of neighbours +1 for current vertex point// total of 9 points in a 3*3 grid
		}
	}
	
	
	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}
}

bool Terrain::GenerateMidpointHeightMap(ID3D11Device* device)
{
	bool result;

	
	int iter = 0; // iter for looping through midpoint passes
	float height = 0.0;

	//we want a wavelength of 1 to be a single wave over the whole terrain.  A single wave is 2 pi which is about 6.283
	m_frequency = (6.283 / m_terrainHeight) / m_wavelength; 
	// m_terrainHeight is actually the z axis

	//loop through the terrain and set the heights how we want. This is where we generate the terrain
	
	/* Initialise corner of height map */ // 1.
	int bottomLeftCorner = 0;
	int bottomRightCorner = (m_terrainHeight * (m_terrainHeight - 1));
	int topLeftCorner = (m_terrainWidth - 1);
	int topRightCorner = m_terrainHeight * (m_terrainHeight - 1) + (m_terrainWidth - 1);

	/* Set height on initial corners*/
	m_heightMap[bottomLeftCorner].y = GenerateRandomNumber(0.1f) * m_amplitude * 5; // 0, 0 (x,z) // bottom left
	m_heightMap[bottomRightCorner].y = GenerateRandomNumber(0.1f) * m_amplitude * 5; // (129 , 0) // bottom right
	m_heightMap[topRightCorner].y = GenerateRandomNumber(0.1f) * m_amplitude * 5; // 129 , 129 // top right
	m_heightMap[topLeftCorner].y = GenerateRandomNumber(0.1f) * m_amplitude * 5; // 0 , 129 // top left

	/* End of Initialise corners of heightmap */

	float leftX, rightX, bottomZ, topZ = 0.f;
	//int test = (int)sqrt((double)m_terrainHeight - 1.0f);
	int heightmapExponent = log(m_terrainWidth - 1) / log(2);
	while (iter < heightmapExponent) // 2. iter < heightmap.exponent
	{
		int chunks = pow(2, iter);
		int chunk_width = (m_terrainHeight - 1) / chunks;

		for (int xChunk = 0; xChunk < chunks; xChunk++)
		{
			for (int yChunk = 0; yChunk < chunks; yChunk++)
			{

				leftX = chunk_width * xChunk;
				rightX = leftX + chunk_width;
				bottomZ = chunk_width * yChunk;
				topZ = bottomZ + chunk_width;
				MidpointDisplace(leftX, rightX, bottomZ, topZ); // coordinates

			}
		}

		iter++;

	}

	result = CalculateNormals();
	if (!result)
	{
		return false;
	}

	result = InitializeBuffers(device);
	if (!result)
	{
		return false;
	}
}

void Terrain::MidpointDisplace(float lx, float rx, float bz, float tz) 
{
	// let
	int index;
	float cx = (lx + rx) / 2.f;
	float cz = (bz + tz) / 2.f;

	// Heights at corners
	float bottomLeftY, bottomRightY, topLeftY, topRightY = 0.f;
	float topMidpointY, leftMidpointY, bottomMidpointY, rightMidpointY, centreMidpointY = 0.f;

	// height value at that point // corners
	// We are using this to find the points on the height map
	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;

			// Bottom Left Corner // Getting Y at this position
			if (m_heightMap[index].x == lx && m_heightMap[index].z == bz) 
			{
				// Get
				bottomLeftY = m_heightMap[index].y;
			}
			// Bottom Right Corner
			if (m_heightMap[index].x == rx && m_heightMap[index].z == bz)
			{
				bottomRightY = m_heightMap[index].y;
			}
			// Top Left Corner
			if (m_heightMap[index].x == lx && m_heightMap[index].z == tz)
			{
				topLeftY = m_heightMap[index].y;
			}
			// Top Right Corner
			if (m_heightMap[index].x == rx && m_heightMap[index].z == tz)
			{
				topRightY = m_heightMap[index].y;
			}
		}
	}
	
	topMidpointY = CalculateAverage(topLeftY, topRightY);
	leftMidpointY = CalculateAverage(bottomLeftY, topLeftY);
	bottomMidpointY = CalculateAverage(bottomLeftY, bottomRightY);
	rightMidpointY = CalculateAverage(bottomRightY, topRightY);
	centreMidpointY = CalculateAverage(topMidpointY, leftMidpointY, bottomMidpointY, rightMidpointY);
	// Once these are done push to midpoint **
	
	// Set Midpoints
	for (int j = 0; j < m_terrainHeight; j++)
	{
		for (int i = 0; i < m_terrainWidth; i++)
		{
			index = (m_terrainHeight * j) + i;
			// Bottom Midpoint
			if (m_heightMap[index].x == cx && m_heightMap[index].z == bz) 
			{
				m_heightMap[index].y = bottomMidpointY * GenerateRandomNumber(0.1f); // jitter spread
			}
			// Top Midpoint
			if (m_heightMap[index].x == cx && m_heightMap[index].z == tz)
			{
				m_heightMap[index].y = topMidpointY * GenerateRandomNumber(0.1f);
			}
			// Left Midpoint
			if (m_heightMap[index].x == lx && m_heightMap[index].z == cz)
			{
				m_heightMap[index].y = leftMidpointY * GenerateRandomNumber(0.1f);
			}
			// Right Midpoint
			if (m_heightMap[index].x == rx && m_heightMap[index].z == cz)
			{
				m_heightMap[index].y = rightMidpointY * GenerateRandomNumber(0.1f);
			}
			// Centre Midpoint
			if (m_heightMap[index].x == cx && m_heightMap[index].z == cz)
			{
				m_heightMap[index].y = centreMidpointY * GenerateRandomNumber(0.1f);
			}
		}
	}

}

float Terrain::CalculateAverage(float a, float b) 
{
	float avg = (a + b) / 2.f;
	return avg;
}

float Terrain::CalculateAverage(float a, float b, float c)
{
	float avg = (a + b + c) / 3.f;
	return avg;
}

float Terrain::CalculateAverage(float a, float b, float c, float d)
{
	float avg = (a + b + c + d) / 4.f;
	return avg;
}

float Terrain::GenerateRandomNumber(float spread)
{
	float x = ((float)rand() / (RAND_MAX));
	return x;
}

bool Terrain::Update()
{
	return true; 
}

float* Terrain::GetWavelength()
{
	return &m_wavelength;
}

float* Terrain::GetAmplitude()
{
	return &m_amplitude;
}
