/*******************************************************************************
Copyright (c) 2005-2009 David Williams

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution. 	
*******************************************************************************/

#include "OpenGLWidget.h"

#include <QMouseEvent>

#include "GradientEstimators.h"
#include "SurfaceExtractor.h"

//Some namespaces we need
using namespace std;
using namespace PolyVox;
using namespace std;

OpenGLWidget::OpenGLWidget(QWidget *parent)
	:QGLWidget(parent)
	,m_volData(0)
{	
	m_xRotation = 0;
	m_yRotation = 0;
	m_uRegionSideLength = 32;

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(0);
}

void OpenGLWidget::setVolume(PolyVox::Volume<PolyVox::uint8_t>* volData)
{
	//First we free anything from the previous volume (if there was one).
	m_mapOpenGLSurfacePatches.clear();
	m_mapIndexedSurfacePatches.clear();
	m_volData = volData;

	//If we have any volume data then generate the new surface patches.
	if(m_volData != 0)
	{
		m_uVolumeWidthInRegions = volData->getWidth() / m_uRegionSideLength;
		m_uVolumeHeightInRegions = volData->getHeight() / m_uRegionSideLength;
		m_uVolumeDepthInRegions = volData->getDepth() / m_uRegionSideLength;

		SurfaceExtractor surfaceExtractor(*volData);
		surfaceExtractor.setLodLevel(0);

		//Our volume is broken down into cuboid regions, and we create one mesh for each region.
		//This three-level for loop iterates over each region.
		for(PolyVox::uint16_t uRegionZ = 0; uRegionZ < m_uVolumeDepthInRegions; ++uRegionZ)
		{
			std::cout << "uRegionZ = " << uRegionZ << " of " << m_uVolumeDepthInRegions << std::endl;
			for(PolyVox::uint16_t uRegionY = 0; uRegionY < m_uVolumeHeightInRegions; ++uRegionY)
			{
				for(PolyVox::uint16_t uRegionX = 0; uRegionX < m_uVolumeWidthInRegions; ++uRegionX)
				{
					//Compute the extents of the current region
					//FIXME - This is a little complex? PolyVox could
					//provide more functions for dealing with regions?
					PolyVox::uint16_t regionStartX = uRegionX * m_uRegionSideLength;
					PolyVox::uint16_t regionStartY = uRegionY * m_uRegionSideLength;
					PolyVox::uint16_t regionStartZ = uRegionZ * m_uRegionSideLength;

					PolyVox::uint16_t regionEndX = regionStartX + m_uRegionSideLength;
					PolyVox::uint16_t regionEndY = regionStartY + m_uRegionSideLength;
					PolyVox::uint16_t regionEndZ = regionStartZ + m_uRegionSideLength;

					Vector3DInt16 regLowerCorner(regionStartX, regionStartY, regionStartZ);
					Vector3DInt16 regUpperCorner(regionEndX, regionEndY, regionEndZ);

					//Extract the surface for this region
					//extractSurface(m_volData, 0, PolyVox::Region(regLowerCorner, regUpperCorner), ispCurrent);
					POLYVOX_SHARED_PTR<IndexedSurfacePatch> isp = surfaceExtractor.extractSurfaceForRegion(PolyVox::Region(regLowerCorner, regUpperCorner));

					//computeNormalsForVertices(m_volData, *(isp.get()), SOBEL_SMOOTHED);
					//*ispCurrent = getSmoothedSurface(*ispCurrent);
					//isp->smooth(0.3f);
					//ispCurrent->generateAveragedFaceNormals(true);

					if(isp->m_vecTriangleIndices.size() > 0)
					{
						isp->makeProgressiveMesh();


						Vector3DUint8 v3dRegPos(uRegionX,uRegionY,uRegionZ);
						if(m_bUseOpenGLVertexBufferObjects)
						{
							OpenGLSurfacePatch openGLSurfacePatch = BuildOpenGLSurfacePatch(*(isp.get()));					
							m_mapOpenGLSurfacePatches.insert(make_pair(v3dRegPos, openGLSurfacePatch));
						}
						//else
						//{
							m_mapIndexedSurfacePatches.insert(make_pair(v3dRegPos, isp));
						//}
						//delete ispCurrent;
					}
				}
			}
		}

		//Projection matrix is dependant on volume size, so we need to set it up again.
		setupProjectionMatrix();
	}
}

void OpenGLWidget::initializeGL()
{
	m_bUseOpenGLVertexBufferObjects = false;
	if(m_bUseOpenGLVertexBufferObjects)
	{
#ifdef WIN32
		//If we are on Windows we will need GLEW to access recent OpenGL functionality
		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			/* Problem: glewInit failed, something is seriously wrong. */
			cout << "Error: " << glewGetErrorString(err) << endl;
		}
#endif
	}

	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glEnable ( GL_COLOR_MATERIAL );
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glEnable(GL_LIGHTING);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_LIGHT0);

	glShadeModel(GL_SMOOTH);
}

void OpenGLWidget::resizeGL(int w, int h)
{
	//Setup the viewport based on the window size
	glViewport(0, 0, w, h);

	//Projection matrix is also dependant on the size of the current volume.
	if(m_volData)
	{
		setupProjectionMatrix();
	}
}

void OpenGLWidget::paintGL()
{
	if(m_volData)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

		glMatrixMode(GL_MODELVIEW);  // Select The Model View Matrix
		glLoadIdentity();									// Reset The Current Modelview Matrix

		//Moves the camera back so we can see the volume
		glTranslatef(0.0f, 0.0f, -m_volData->getDiagonalLength());	

		glRotatef(m_xRotation, 1.0f, 0.0f, 0.0f);
		glRotatef(m_yRotation, 0.0f, 1.0f, 0.0f);

		//Centre the volume on the origin
		glTranslatef(-g_uVolumeSideLength/2,-g_uVolumeSideLength/2,-g_uVolumeSideLength/2);

		for(PolyVox::uint16_t uRegionZ = 0; uRegionZ < m_uVolumeDepthInRegions; ++uRegionZ)
		{
			for(PolyVox::uint16_t uRegionY = 0; uRegionY < m_uVolumeHeightInRegions; ++uRegionY)
			{
				for(PolyVox::uint16_t uRegionX = 0; uRegionX < m_uVolumeWidthInRegions; ++uRegionX)
				{
					Vector3DUint8 v3dRegPos(uRegionX,uRegionY,uRegionZ);
					if(m_mapIndexedSurfacePatches.find(v3dRegPos) != m_mapIndexedSurfacePatches.end())
					{
						POLYVOX_SHARED_PTR<IndexedSurfacePatch> ispCurrent = m_mapIndexedSurfacePatches[v3dRegPos];
						unsigned int uLodLevel = 0; //ispCurrent->m_vecLodRecords.size() - 1;
						if(m_bUseOpenGLVertexBufferObjects)
						{													
							renderRegionVertexBufferObject(m_mapOpenGLSurfacePatches[v3dRegPos], uLodLevel);
						}
						else
						{						
							renderRegionImmediateMode(*ispCurrent, uLodLevel);
						}
					}
				}
			}
		}

		GLenum errCode;
		const GLubyte *errString;

		if ((errCode = glGetError()) != GL_NO_ERROR)
		{
			errString = gluErrorString(errCode);
			cout << "OpenGL Error: " << errString << endl;
		}
	}
}

void OpenGLWidget::mousePressEvent(QMouseEvent* event)
{
	m_CurrentMousePos = event->pos();
	m_LastFrameMousePos = m_CurrentMousePos;
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent* event)
{
	m_CurrentMousePos = event->pos();
	QPoint diff = m_CurrentMousePos - m_LastFrameMousePos;
	m_xRotation += diff.x();
	m_yRotation += diff.y();
	m_LastFrameMousePos = m_CurrentMousePos;;
}

void OpenGLWidget::wheelEvent(QWheelEvent* event)
{
}

void OpenGLWidget::setupProjectionMatrix(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float frustumSize = m_volData->getDiagonalLength() / 2.0f;
	float aspect = static_cast<float>(width()) / static_cast<float>(height());

	glOrtho(frustumSize*aspect, -frustumSize*aspect, frustumSize, -frustumSize, 1.0, 5000);
}