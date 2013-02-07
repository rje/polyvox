#! /usr/bin/env python

################################################################################
# Copyright (c) 2013 Matt Williams
# 
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
# 
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
# 
#     1. The origin of this software must not be misrepresented; you must not
#     claim that you wrote the original software. If you use this software
#     in a product, an acknowledgment in the product documentation would be
#     appreciated but is not required.
# 
#     2. Altered source versions must be plainly marked as such, and must not be
#     misrepresented as being the original software.
# 
#     3. This notice may not be removed or altered from any source
#     distribution.
################################################################################

import sys
sys.path.append("../../library/bindings/") #This is just to point to the generated bindings

import PolyVoxCore as pv

#Create a 64x64x64 volume of integers
r = pv.Region(pv.Vector3Dint32_t(0,0,0), pv.Vector3Dint32_t(63,63,63))
vol = pv.SimpleVolumeuint8(r)

#Now fill the volume with our data (a sphere)
v3dVolCenter = pv.Vector3Dint32_t(vol.getWidth() / 2, vol.getHeight() / 2, vol.getDepth() / 2)
sphereRadius = 30
#This three-level for loop iterates over every voxel in the volume
for z in range(vol.getDepth()):
	for y in range(vol.getHeight()):
		for x in range(vol.getWidth()):
			#Store our current position as a vector...
			v3dCurrentPos = pv.Vector3Dint32_t(x,y,z)
			#And compute how far the current position is from the center of the volume
			fDistToCenter = (v3dCurrentPos - v3dVolCenter).length()
			
			uVoxelValue = 0
			
			#If the current voxel is less than 'radius' units from the center then we make it solid.
			if(fDistToCenter <= sphereRadius):
				#Our new voxel value
				uVoxelValue = 255
			
			#Write the voxel value into the volume
			vol.setVoxelAt(x, y, z, uVoxelValue);

#Create a mesh, pass it to the extractor and generate the mesh
mesh = pv.SurfaceMeshPositionMaterialNormal()
extractor = pv.CubicSurfaceExtractorWithNormalsSimpleVolumeuint8(vol, r, mesh)
extractor.execute()

#That's all of the PolyVox generation done, now to convert the output to something OpenGL can read efficiently

import numpy as np

indices = np.array(mesh.getIndices(), dtype='uint32') #Throw in the vertex indices into an array
#The vertices and normals are placed in an interpolated array like [vvvnnn,vvvnnn,vvvnnn]
vertices = np.array([[vertex.getPosition().getX(), vertex.getPosition().getY(), vertex.getPosition().getZ(),
                      vertex.getNormal().getX(), vertex.getNormal().getY(), vertex.getNormal().getZ()]
                      for vertex in mesh.getVertices()],
                    dtype='f')

#Now that we have our data, everything else here is just OpenGL

import OpenGL

from OpenGL.GL import shaders
from OpenGL.arrays import vbo
from OpenGL.GL import glClear, glEnable, glDepthFunc, GLuint, glEnableVertexAttribArray, glVertexAttribPointer, glDisableVertexAttribArray, \
                      glDrawElements, glGetUniformLocation, glUniformMatrix4fv, glDepthMask, glDepthRange, glGetString, glBindAttribLocation, \
                      GL_COLOR_BUFFER_BIT, GL_TRIANGLES, GL_DEPTH_TEST, GL_LEQUAL, GL_FLOAT, \
                      GL_DEPTH_BUFFER_BIT, GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW, \
                      GL_FALSE, GL_TRUE, GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_CULL_FACE, \
                      GL_VENDOR, GL_RENDERER, GL_VERSION, GL_SHADING_LANGUAGE_VERSION
from OpenGL.raw.GL.ARB.vertex_array_object import glGenVertexArrays, glBindVertexArray

import pygame

from math import sin, cos, tan, radians

SCREEN_SIZE = (800, 800)

def run():
	#Start OpenGL and ask it for an OpenGL context
	pygame.init()
	clock = pygame.time.Clock()
	screen = pygame.display.set_mode(SCREEN_SIZE, pygame.HWSURFACE|pygame.OPENGL|pygame.DOUBLEBUF)
	
	#The first thing we do is print some OpenGL details and check that we have a good enough version
	print "OpenGL Implementation Details:"
	if glGetString(GL_VENDOR):
		print "\tGL_VENDOR:", glGetString(GL_VENDOR)
	if glGetString(GL_RENDERER):
		print "\tGL_RENDERER:", glGetString(GL_RENDERER)
	if glGetString(GL_VERSION):
		print "\tGL_VERSION:", glGetString(GL_VERSION)
	if glGetString(GL_SHADING_LANGUAGE_VERSION):
		print "\tGL_SHADING_LANGUAGE_VERSION:", glGetString(GL_SHADING_LANGUAGE_VERSION)
	
	major_version = int(glGetString(GL_VERSION).split()[0].split('.')[0])
	minor_version = int(glGetString(GL_VERSION).split()[0].split('.')[1])
	if major_version < 3 or (major_version < 3 and minor_version < 0):
		print "OpenGL version must be at least 3.0 (found {0})".format(glGetString(GL_VERSION).split()[0])
	
	#Now onto the OpenGL initialisation
	
	#Set up depth culling
	glEnable(GL_CULL_FACE)
	glEnable(GL_DEPTH_TEST)
	glDepthMask(GL_TRUE)
	glDepthFunc(GL_LEQUAL)
	glDepthRange(0.0, 1.0)
	
	#We create out shaders which do little more than set a flat colour for each face
	
	VERTEX_SHADER = shaders.compileShader("""
	#version 130
	
	in vec4 position;
	in vec4 normal;
	
	uniform mat4 cameraToClipMatrix;
	uniform mat4 worldToCameraMatrix;
	uniform mat4 modelToWorldMatrix;
	
	flat out float theColor;
	
	void main()
	{
		vec4 temp = modelToWorldMatrix * position;
		temp = worldToCameraMatrix * temp;
		gl_Position = cameraToClipMatrix * temp;
		
		theColor = clamp(abs(dot(normalize(normal.xyz), normalize(vec3(0.9,0.1,0.5)))), 0, 1);
	}
	""", GL_VERTEX_SHADER)
	
	FRAGMENT_SHADER = shaders.compileShader("""
	#version 130
	
	flat in float theColor;
	
	out vec4 outputColor;
	void main()
	{
		outputColor = vec4(1.0, 0.5, theColor, 1.0);
	}
	""", GL_FRAGMENT_SHADER)
	
	shader = shaders.compileProgram(VERTEX_SHADER, FRAGMENT_SHADER)
	
	#And then grab our attribute locations from it
	glBindAttribLocation(shader, 0, "position")
	glBindAttribLocation(shader, 1, "normal")
	
	#Create the Vertex Array Object to hold our volume mesh
	vertexArrayObject = GLuint(0)
	glGenVertexArrays(1, vertexArrayObject)
	glBindVertexArray(vertexArrayObject)
	
	#Create the index buffer object
	indexPositions = vbo.VBO(indices, target=GL_ELEMENT_ARRAY_BUFFER, usage=GL_STATIC_DRAW)
	#Create the VBO
	vertexPositions = vbo.VBO(vertices, usage=GL_STATIC_DRAW)
	
	#Bind our VBOs and set up our data layout specifications
	with indexPositions, vertexPositions:
		glEnableVertexAttribArray(0)
		glVertexAttribPointer(0, 3, GL_FLOAT, False, 6*vertices.dtype.itemsize, vertexPositions+(0*vertices.dtype.itemsize))
		glEnableVertexAttribArray(1)
		glVertexAttribPointer(1, 3, GL_FLOAT, False, 6*vertices.dtype.itemsize, vertexPositions+(3*vertices.dtype.itemsize))
		
		glBindVertexArray(0)
		glDisableVertexAttribArray(0)
	
	#Now grab out transformation martix locations
	modelToWorldMatrixUnif = glGetUniformLocation(shader, "modelToWorldMatrix")
	worldToCameraMatrixUnif = glGetUniformLocation(shader, "worldToCameraMatrix")
	cameraToClipMatrixUnif = glGetUniformLocation(shader, "cameraToClipMatrix")
	
	modelToWorldMatrix = np.array([[1.0,0.0,0.0,-32.0],[0.0,1.0,0.0,-32.0],[0.0,0.0,1.0,-32.0],[0.0,0.0,0.0,1.0]], dtype='f')
	worldToCameraMatrix = np.array([[1.0,0.0,0.0,0.0],[0.0,1.0,0.0,0.0],[0.0,0.0,1.0,-50.0],[0.0,0.0,0.0,1.0]], dtype='f')
	cameraToClipMatrix = np.array([[0.0,0.0,0.0,0.0],[0.0,0.0,0.0,0.0],[0.0,0.0,0.0,0.0],[0.0,0.0,0.0,0.0]], dtype='f')
	
	#These next few lines just set up our camera frustum
	fovDeg = 45.0
	frustumScale = 1.0 / tan(radians(fovDeg) / 2.0)
	
	zNear = 1.0
	zFar = 1000.0
	
	cameraToClipMatrix[0][0] = frustumScale
	cameraToClipMatrix[1][1] = frustumScale
	cameraToClipMatrix[2][2] = (zFar + zNear) / (zNear - zFar)
	cameraToClipMatrix[2][3] = -1.0
	cameraToClipMatrix[3][2] = (2 * zFar * zNear) / (zNear - zFar)
	
	#worldToCameraMatrix and cameraToClipMatrix don't change ever so just set them once here
	with shader:
		glUniformMatrix4fv(cameraToClipMatrixUnif, 1, GL_TRUE, cameraToClipMatrix)
		glUniformMatrix4fv(worldToCameraMatrixUnif, 1, GL_TRUE, worldToCameraMatrix)
	
	#These are used to track the rotation of the volume
	LastFrameMousePos = (0,0)
	CurrentMousePos = (0,0)
	xRotation = 0
	yRotation = 0
	
	while True:
		clock.tick()
		
		for event in pygame.event.get():
			if event.type == pygame.QUIT:
				return
			if event.type == pygame.KEYUP and event.key == pygame.K_ESCAPE:
				return
			if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
				CurrentMousePos = event.pos
				LastFrameMousePos = CurrentMousePos
			if event.type == pygame.MOUSEMOTION and 1 in event.buttons:
				CurrentMousePos = event.pos
				diff = (CurrentMousePos[0] - LastFrameMousePos[0], CurrentMousePos[1] - LastFrameMousePos[1])
				xRotation += event.rel[0]
				yRotation += event.rel[1]
				LastFrameMousePos = CurrentMousePos
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
		
		#Perform the rotation of the mesh
		moveToOrigin = np.array([[1.0,0.0,0.0,-32.0],[0.0,1.0,0.0,-32.0],[0.0,0.0,1.0,-32.0],[0.0,0.0,0.0,1.0]], dtype='f')
		rotateAroundX = np.array([[1.0,0.0,0.0,0.0],[0.0,cos(radians(yRotation)),-sin(radians(yRotation)),0.0],[0.0,sin(radians(yRotation)),cos(radians(yRotation)),0.0],[0.0,0.0,0.0,1.0]], dtype='f')
		rotateAroundY = np.array([[cos(radians(xRotation)),0.0,sin(radians(xRotation)),0.0],[0.0,1.0,0.0,0.0],[-sin(radians(xRotation)),0.0,cos(radians(xRotation)),0.0],[0.0,0.0,0.0,1.0]], dtype='f')
		
		modelToWorldMatrix = rotateAroundY.dot(rotateAroundX.dot(moveToOrigin))
		
		with shader:
			glUniformMatrix4fv(modelToWorldMatrixUnif, 1, GL_TRUE, modelToWorldMatrix)
			glBindVertexArray(vertexArrayObject)
			
			glDrawElements(GL_TRIANGLES, len(indices), GL_UNSIGNED_INT, None)
			
			glBindVertexArray(0)
		
		# Show the screen
		pygame.display.flip()

run()
