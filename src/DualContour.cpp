#include "DualContour.h"


DualContour::DualContour() {
}


DualContour::~DualContour() {
}

Mesh DualContour::ExtractSurface(float(*function)(float, float, float)) {
	//TODO:Octree should be created here.

	const float halfStep = STEP / 2.0;
	const glm::fvec3 cornerOffset[8] = {
		glm::fvec3(0,0,0),
		glm::fvec3(0,0,1),
		glm::fvec3(0,1,0),
		glm::fvec3(0,1,1),
		glm::fvec3(1,0,0),
		glm::fvec3(1,0,1),
		glm::fvec3(1,1,0),
		glm::fvec3(1,1,1)
	};


	const int edgeMap[12][2] = {
		{0,4},{1,5},{2,6},{3,7},	// x-axis 
		{0,2},{1,3},{4,6},{5,7},	// y-axis
		{0,1},{2,3},{4,5},{6,7}		// z-axis
	};

	const int size = MAX - MIN;
	TreeNode nodeArray[size][size][size];

	unsigned int index = 0;
	for (int x = MIN; x < MAX; x++) {
		for (int y = MIN; y < MAX; y++) {
			for (int z = MIN; z < MAX; z++) {

				/*
				The variable cornerSum is an 8 bit unsigned int(takes values on the range [0,255]).We use this
				in the place of a bool array in order to determine whether a corner of a cube has a value or not 
				based on the function threshold.Each bit corresponds to one corner.
				*/
				uint8_t cornerHasSurface = 0;
				float cornerDensity[8];
				for(int i=0;i<8;i++){
					glm::fvec3 cornerPosition = glm::fvec3(x, y, z) + cornerOffset[i];
					cornerDensity[i] = function(cornerPosition.x,cornerPosition.y,cornerPosition.z);

					//If the density is below the threshold, the corner is outside the shape.
					int hasSurface = cornerDensity[i] < THRESHOLD ? 0 : 1;
					cornerHasSurface |= (hasSurface << i);
				}


				if(cornerHasSurface == 0 || cornerHasSurface == 255){
					//Delete the node because it is either fully outside the function or fully inside it.
					nodeArray[x - MIN][y - MIN][z - MIN].cubeValid = false;
					continue;
				}

				std::vector<glm::fvec3> edgeList;
				for (unsigned int i = 0; i < 12; i++) {
					//Get the two vertices(the corners) of the edge.
					int v0 = (cornerHasSurface >> edgeMap[i][0]) & 1;
					int v1 = (cornerHasSurface >> edgeMap[i][1]) & 1;

					//If they are not equal,means that there was a sign change somewhere on this edge.
					if(v0 != v1){

						//We could get the other corner as well but its not necessary.
						glm::fvec3 edge = glm::fvec3(x, y, z) + cornerOffset[edgeMap[i][0]];

						float c0Value = cornerDensity[edgeMap[i][0]];
						float c1Value = cornerDensity[edgeMap[i][1]];

						//TODO:These are not optimal.
						if(i<4){
							edge.x = x + Adapt(c0Value, c1Value);
						}else if(i<8){
							edge.y = y + Adapt(c0Value, c1Value);
						} else {
							edge.z = z + Adapt(c0Value, c1Value);
						}

						//Add it to the list.
						edgeList.push_back(edge);
					}
				}

				//Calculate edge normal and add it to a list.
				std::vector<glm::fvec3> edgeNormals;
				for(unsigned int i=0;i<edgeList.size();i++){
					glm::fvec3 normalValue = CalculateNormal(function, edgeList[i]);
					edgeNormals.push_back(normalValue);
				}


				//Solve QEF.
				glm::fvec3 pos = Qef::Solve(glm::fvec3(x, y, z), edgeList, edgeNormals);
				nodeArray[x - MIN][y - MIN][z - MIN].pos = pos;
				std::cout << "Pos : [" << pos.x << "," << pos.y << "," << pos.z << "]\n";
				//Assign index to vertex.
				nodeArray[x - MIN][y - MIN][z - MIN].index = index++;

				//Mark which of the 3 (important) edges have a sign change.
				int edge37 = ((cornerHasSurface >> 3) & 1) != ((cornerHasSurface >> 7) & 1) ? 1 : 0;
				nodeArray[x - MIN][y - MIN][z - MIN].edgeHasSignChange |= (edge37 << 0);
				int edge57 = ((cornerHasSurface >> 5) & 1) != ((cornerHasSurface >> 7) & 1) ? 1 : 0;
				nodeArray[x - MIN][y - MIN][z - MIN].edgeHasSignChange |= (edge37 << 1);
				int edge67 = ((cornerHasSurface >> 6) & 1) != ((cornerHasSurface >> 7) & 1) ? 1 : 0;
				nodeArray[x - MIN][y - MIN][z - MIN].edgeHasSignChange |= (edge37 << 2);

				if(nodeArray[x - MIN][y - MIN][z - MIN].edgeHasSignChange == 0){
					std::cout << "Is 0\n";
				}else{
					std::cout << "Is " << unsigned(nodeArray[x - MIN][y - MIN][z - MIN].edgeHasSignChange) << std::endl;
				}

				//Push the coords to the vertex vector(will be converted to array later).
				glm::fvec3 normPos = Normalize(pos);
				vertArray.push_back(normPos.x);
				vertArray.push_back(normPos.y);
				vertArray.push_back(normPos.z);
				
				std::cout <<"[" << x-MIN << "," << y - MIN << "," << z-MIN << "] : " << "Pushed " << normPos.x << "," << normPos.y << "," << normPos.z << " to the vertArray.\n";
				
			}
		}
	}
	
	for (int x = MIN; x < MAX-1; x++) {
		for (int y = MIN; y < MAX-1; y++) {
			for (int z = MIN; z < MAX-1; z++) {
				bool solid1 = function(x+1, y+1, z) > 0;
				bool solid2 = function(x + 1, y + 1, z + 1) > 0;
				if(solid1 != solid2){
					//TODO:Build triangles.

					unsigned int x0y0 = nodeArray[x - MIN][y - MIN][z - MIN].index;
					unsigned int x0y1 = nodeArray[x - MIN][y - MIN + 1][z - MIN].index;
					unsigned int x1y0 = nodeArray[x - MIN + 1][y - MIN][z - MIN].index;
					unsigned int x1y1 = nodeArray[x - MIN + 1][y - MIN + 1][z - MIN].index;

					nodeArray[x - MIN][y - MIN][z - MIN].used = true;
					nodeArray[x - MIN][y - MIN + 1][z - MIN].used = true;
					nodeArray[x - MIN + 1][y - MIN][z - MIN].used = true;
					nodeArray[x - MIN + 1][y - MIN + 1][z - MIN].used = true;

					indices.push_back(x1y1);
					indices.push_back(x1y0);
					indices.push_back(x0y1);

					indices.push_back(x1y0);
					indices.push_back(x0y0);
					indices.push_back(x0y1);

				}
				solid1 = function(x + 1, y, z+1) > 0;
				solid2 = function(x + 1, y + 1, z + 1) > 0;
				if(solid1 != solid2){

					unsigned int x0z0 = nodeArray[x - MIN][y - MIN][z - MIN].index;
					unsigned int x0z1 = nodeArray[x - MIN][y - MIN][z - MIN + 1].index;
					unsigned int x1z0 = nodeArray[x - MIN + 1][y - MIN][z - MIN].index;
					unsigned int x1z1 = nodeArray[x - MIN + 1][y - MIN ][z - MIN + 1].index;
				
					nodeArray[x - MIN][y - MIN][z - MIN].used = true;
					nodeArray[x - MIN][y - MIN][z - MIN + 1].used = true;
					nodeArray[x - MIN + 1][y - MIN][z - MIN].used = true;
					nodeArray[x - MIN + 1][y - MIN][z - MIN + 1].used = true;

					indices.push_back(x1z1);
					indices.push_back(x1z0);
					indices.push_back(x0z1);

					indices.push_back(x1z0);
					indices.push_back(x0z0);
					indices.push_back(x0z1);

				}
				solid1 = function(x, y + 1, z + 1) > 0;
				solid2 = function(x + 1, y + 1, z + 1) > 0;
				if (solid1 != solid2) {

					unsigned int y0z0 = nodeArray[x - MIN][y - MIN][z - MIN].index;
					unsigned int y0z1 = nodeArray[x - MIN][y - MIN][z - MIN + 1].index;
					unsigned int y1z0 = nodeArray[x - MIN][y - MIN + 1][z - MIN].index;
					unsigned int y1z1 = nodeArray[x - MIN][y - MIN + 1][z - MIN + 1].index;

					nodeArray[x - MIN][y - MIN][z - MIN].used = true;
					nodeArray[x - MIN][y - MIN][z - MIN + 1].used = true;
					nodeArray[x - MIN][y - MIN + 1][z - MIN].used = true;
					nodeArray[x - MIN][y - MIN + 1][z - MIN + 1].used = true;

					indices.push_back(y1z1);
					indices.push_back(y1z0);
					indices.push_back(y0z1);

					indices.push_back(y1z0);
					indices.push_back(y0z0);
					indices.push_back(y0z1);

				}

			}
		}
	}

	for (int x = MIN; x < MAX - 1; x++) {
		for (int y = MIN; y < MAX - 1; y++) {
			for (int z = MIN; z < MAX - 1; z++) {
				if(nodeArray[x - MIN][y - MIN][z - MIN].cubeValid && !nodeArray[x - MIN][y - MIN][z - MIN].used){
					glm::fvec3 p = Normalize(nodeArray[x - MIN][y - MIN][z - MIN].pos);
					std::cout << "Not used: "<< p.x << "," << p.y << "," << p.z << std::endl;
				}
			}
		}
	}

	std::cout << "Vertices\n";
	for (int i = 0; i < vertArray.size(); i+= 3) {
		std::cout << vertArray[i] << " " << vertArray[i+1] << " " <<  vertArray[i + 2] << "\n";
	}

	std::cout << "Indices\n";
	for (int i = 0; i < indices.size(); i+= 3) {
		std::cout << indices[i] << " " << indices[i+1] << " " << indices[i+2] << "\n";
	}
	VertexBuffer vertexBuffer(&vertArray[0],vertArray.size() * sizeof(float));
	
	VertexBufferLayout bufferLayout;
	bufferLayout.Push<float>(3, true);
	
	VertexArray vertexArray;
	vertexArray.AddBuffer(vertexBuffer, bufferLayout);

	IndexBuffer indexBuffer(&indices[0],indices.size());
	Mesh mesh(vertexArray, indexBuffer);
	return mesh;

}

float DualContour::Adapt(float v0Val, float v1Val) {
	return -v0Val /(v1Val - v0Val);
}

glm::fvec3 DualContour::CalculateNormal(float(*function)(float, float, float),glm::fvec3 pos, float d) {
	glm::fvec3 derivatives;

	derivatives.x =(function(pos.x + d, pos.y, pos.z) - function(pos.x - d, pos.y, pos.z))/2.0/d;
	derivatives.y =(function(pos.x, pos.y + d, pos.z) - function(pos.x, pos.y - d, pos.z)) / 2.0 / d;
	derivatives.z =(function(pos.x, pos.y, pos.z + d) - function(pos.x, pos.y, pos.z - d)) / 2.0 / d;

	return glm::normalize(derivatives);
}

glm::fvec3 DualContour::Normalize(glm::fvec3 v) {
	//Get x^2,y^2,z^2.
	float x = v.x * v.x;
	float y = v.y * v.y;
	float z = v.z * v.z;
	
	//Calculate magnitude.
	float m = sqrt(x + y + z);

	//Finally,normalize by dividing with magnitude.
	return glm::fvec3(v.x/m,v.y/m,v.z/m);
}









