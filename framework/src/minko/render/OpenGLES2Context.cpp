/*
Copyright (c) 2013 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "minko/render/OpenGLES2Context.hpp"

#include <iomanip>
#include "minko/render/CompareMode.hpp"
#include "minko/render/WrapMode.hpp"
#include "minko/render/TextureFilter.hpp"
#include "minko/render/MipFilter.hpp"
#include "minko/render/TriangleCulling.hpp"
#include "minko/render/StencilOperation.hpp"

#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
# include <OpenGL/gl.h>
# include <GLUT/glut.h>
#elif MINKO_ANGLE
# include "GLES2/gl2.h"
# include "minko/math/Matrix4x4.hpp"
#elif _WIN32
# include "GL/glew.h"
#elif EMSCRIPTEN
# include <GLES2/gl2.h>
# include <EGL/egl.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif

#ifdef MINKO_GLSL_OPTIMIZER
# include "glsl_optimizer.h"
#endif

using namespace minko;
using namespace minko::render;

OpenGLES2Context::BlendFactorsMap OpenGLES2Context::_blendingFactors = OpenGLES2Context::initializeBlendFactorsMap();
OpenGLES2Context::BlendFactorsMap
OpenGLES2Context::initializeBlendFactorsMap()
{
    BlendFactorsMap m;

    m[static_cast<uint>(Blending::Source::ZERO)]                       = GL_ZERO;
    m[static_cast<uint>(Blending::Source::ONE)]                        = GL_ONE;
    m[static_cast<uint>(Blending::Source::SRC_COLOR)]                  = GL_SRC_COLOR;
    m[static_cast<uint>(Blending::Source::ONE_MINUS_SRC_COLOR)]        = GL_ONE_MINUS_SRC_COLOR;
    m[static_cast<uint>(Blending::Source::SRC_ALPHA)]                  = GL_SRC_ALPHA;
    m[static_cast<uint>(Blending::Source::ONE_MINUS_SRC_ALPHA)]        = GL_ONE_MINUS_SRC_ALPHA;
    m[static_cast<uint>(Blending::Source::DST_ALPHA)]                  = GL_DST_ALPHA;
    m[static_cast<uint>(Blending::Source::ONE_MINUS_DST_ALPHA)]        = GL_ONE_MINUS_DST_ALPHA;

    m[static_cast<uint>(Blending::Destination::ZERO)]                  = GL_ZERO;
    m[static_cast<uint>(Blending::Destination::ONE)]                   = GL_ONE;
    m[static_cast<uint>(Blending::Destination::DST_COLOR)]             = GL_DST_COLOR;
    m[static_cast<uint>(Blending::Destination::ONE_MINUS_DST_COLOR)]   = GL_ONE_MINUS_DST_COLOR;
    m[static_cast<uint>(Blending::Destination::ONE_MINUS_DST_ALPHA)]   = GL_ONE_MINUS_DST_ALPHA;
    m[static_cast<uint>(Blending::Destination::ONE_MINUS_SRC_ALPHA)]   = GL_ONE_MINUS_SRC_ALPHA;
    m[static_cast<uint>(Blending::Destination::DST_ALPHA)]             = GL_DST_ALPHA;
    m[static_cast<uint>(Blending::Destination::ONE_MINUS_DST_ALPHA)]   = GL_ONE_MINUS_DST_ALPHA;

    return m;
}

OpenGLES2Context::CompareFuncsMap OpenGLES2Context::_compareFuncs = OpenGLES2Context::initializeDepthFuncsMap();
OpenGLES2Context::CompareFuncsMap
OpenGLES2Context::initializeDepthFuncsMap()
{
	CompareFuncsMap m;

	m[CompareMode::ALWAYS]			= GL_ALWAYS;
	m[CompareMode::EQUAL]			= GL_EQUAL;
	m[CompareMode::GREATER]			= GL_GREATER;
	m[CompareMode::GREATER_EQUAL]	= GL_GEQUAL;
	m[CompareMode::LESS]			= GL_LESS;
	m[CompareMode::LESS_EQUAL]		= GL_LEQUAL;
	m[CompareMode::NEVER]			= GL_NEVER;
	m[CompareMode::NOT_EQUAL]		= GL_NOTEQUAL;

	return m;
}

OpenGLES2Context::StencilOperationMap OpenGLES2Context::_stencilOps = OpenGLES2Context::initializeStencilOperationsMap();
OpenGLES2Context::StencilOperationMap
OpenGLES2Context::initializeStencilOperationsMap()
{
	StencilOperationMap m;

	m[StencilOperation::KEEP]		= GL_KEEP;
	m[StencilOperation::ZERO]		= GL_ZERO;
	m[StencilOperation::REPLACE]	= GL_REPLACE;
	m[StencilOperation::INCR]		= GL_INCR;
	m[StencilOperation::INCR_WRAP]	= GL_INCR_WRAP;
	m[StencilOperation::DECR]		= GL_DECR;
	m[StencilOperation::DECR_WRAP]	= GL_DECR_WRAP;
	m[StencilOperation::INVERT]		= GL_INVERT;

	return m;
}

OpenGLES2Context::OpenGLES2Context() :
	_errorsEnabled(false),
	_textures(),
    _textureSizes(),
    _textureHasMipmaps(),
	_viewportX(0),
	_viewportY(0),
	_viewportWidth(0),
	_viewportHeight(0),
	_currentTarget(0),
	_currentIndexBuffer(0),
	_currentVertexBuffer(8, 0),
	_currentVertexSize(8, -1),
	_currentVertexStride(8, -1),
	_currentVertexOffset(8, -1),
	_currentTexture(8, 0),
	_currentProgram(0),
    _currentTriangleCulling(TriangleCulling::BACK),
    _currentWrapMode(),
    _currentTextureFilter(),
    _currentMipFilter(),
    _currentBlendMode(Blending::Mode::DEFAULT),
	_currentColorMask(true),
    _currentDepthMask(true),
    _currentDepthFunc(CompareMode::UNSET),
	_currentStencilFunc(CompareMode::UNSET),
	_currentStencilRef(0),
	_currentStencilMask(0x1),
	_currentStencilFailOp(StencilOperation::UNSET),
	_currentStencilZFailOp(StencilOperation::UNSET),
	_currentStencilZPassOp(StencilOperation::UNSET)
{
#if defined _WIN32 && !defined MINKO_ANGLE
    glewInit();
#endif

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    _driverInfo = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)))
        + " " + std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)))
        + " " + std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));

	// init. viewport x, y, width and height
	std::vector<int> viewportSettings(4);
	glGetIntegerv(GL_VIEWPORT, &viewportSettings[0]);
	_viewportX = viewportSettings[0];
	_viewportY = viewportSettings[1];
	_viewportWidth = viewportSettings[2];
	_viewportHeight = viewportSettings[3];

	setColorMask(true);
	setDepthTest(true, CompareMode::LESS);
	setStencilTest(CompareMode::ALWAYS, 0, 0x1, StencilOperation::KEEP, StencilOperation::KEEP, StencilOperation::KEEP);
}

OpenGLES2Context::~OpenGLES2Context()
{
	for (auto& vertexBuffer : _vertexBuffers)
		glDeleteBuffers(1, &vertexBuffer);

	for (auto& indexBuffer : _indexBuffers)
		glDeleteBuffers(1, &indexBuffer);

	for (auto& texture : _textures)
		deleteTexture(texture);

	for (auto& program : _programs)
		glDeleteProgram(program);

	for (auto& vertexShader : _vertexShaders)
		glDeleteShader(vertexShader);

	for (auto& fragmentShader : _fragmentShaders)
		glDeleteShader(fragmentShader);
}

void
OpenGLES2Context::configureViewport(const uint x,
				  				    const uint y,
				  				    const uint width,
				  				    const uint height)
{
	if (x != _viewportX || y != _viewportY || width != _viewportWidth || height != _viewportHeight)
	{
		_viewportX = x;
		_viewportY = y;
		_viewportWidth = width;
		_viewportHeight = height;

		glViewport(x, y, width, height);
	}
}

void
OpenGLES2Context::clear(float 	red,
					    float 	green,
					    float 	blue,
					    float 	alpha,
					    float 	depth,
					    uint 	stencil,
					    uint 	mask)
{
	// http://www.opengl.org/sdk/docs/man/xhtml/glClearColor.xml
	//
	// void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	// red, green, blue, alpha Specify the red, green, blue, and alpha values used when the color buffers are cleared.
	// The initial values are all 0.
	//
	// glClearColor specify clear values for the color buffers
	glClearColor(red, green, blue, alpha);

	// http://www.opengl.org/sdk/docs/man/xhtml/glClearDepth.xml
	//
	// void glClearDepth(GLdouble depth);
	// void glClearDepthf(GLfloat depth);
	// depth Specifies the depth value used when the depth buffer is cleared. The initial value is 1.
	//
	// glClearDepth specify the clear value for the depth buffer
#ifdef GL_ES_VERSION_2_0
	glClearDepthf(depth);
#else
	glClearDepth(depth);
#endif

	// http://www.opengl.org/sdk/docs/man/xhtml/glClearStencil.xml
	//
	// void glClearStencil(GLint s)
	// Specifies the index used when the stencil buffer is cleared. The initial value is 0.
	//
	// glClearStencil specify the clear value for the stencil buffer
#ifndef MINKO_NO_STENCIL
	glClearStencil(stencil);
#endif

	// http://www.opengl.org/sdk/docs/man/xhtml/glClear.xml
	//
	// void glClear(GLbitfield mask);
	// mask
	// Bitwise OR of masks that indicate the buffers to be cleared. The three masks are GL_COLOR_BUFFER_BIT,
	// GL_DEPTH_BUFFER_BIT, and GL_STENCIL_BUFFER_BIT.
	//
	// glClear clear buffers to preset values
	mask = (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT) & mask;
	if (mask & GL_DEPTH_BUFFER_BIT)
		glDepthMask(_currentDepthMask = true);
	glClear(mask);
}

void
OpenGLES2Context::present()
{
	// http://www.opengl.org/sdk/docs/man/xhtml/glFlush.xml
	//
	// force execution of GL commands in finite time
	//glFlush();
	
    setRenderToBackBuffer();
}

void
OpenGLES2Context::drawTriangles(const uint indexBuffer, const int numTriangles)
{
	if (_currentIndexBuffer != indexBuffer)
	{
		_currentIndexBuffer = indexBuffer;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	}

	// http://www.opengl.org/sdk/docs/man/xhtml/glDrawElements.xml
	//
	// void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
	// mode Specifies what kind of primitives to render.
	// count Specifies the number of elements to be rendered.
	// type Specifies the type of the values in indices.
	// indices Specifies a pointer to the location where the indices are stored.
	//
	// glDrawElements render primitives from array data
	glDrawElements(GL_TRIANGLES, numTriangles * 3, GL_UNSIGNED_SHORT, (void*)0);

    checkForErrors();
}

const uint
OpenGLES2Context::createVertexBuffer(const uint size)
{
	uint vertexBuffer;

	// http://www.opengl.org/sdk/docs/man/xhtml/glGenBuffers.xml
	//
	// void glGenBuffers(GLsizei n, GLuint* buffers);
	// n Specifies the number of buffer object names to be vertexBufferd.
	// buffers Specifies an array in which the generated buffer object names are stored.
	//
	// glGenBuffers returns n buffer object names in buffers. There is no
	// guarantee that the names form a contiguous set of integers; however,
	// it is guaranteed that none of the returned names was in use immediately
	// before the call to glGenBuffers.
	glGenBuffers(1, &vertexBuffer);

	// http://www.opengl.org/sdk/docs/man/xhtml/glBindBuffer.xml
	//
	// void glBindBuffer(GLenum target, GLuint buffer);
	// target Specifies the target to which the buffer object is bound.
	// buffer Specifies the name of a buffer object.
	//
	// glBindBuffer binds a buffer object to the specified buffer binding point.
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	// http://www.opengl.org/sdk/docs/man/xhtml/glBufferData.xml
	//
	// void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
	// target Specifies the target buffer object.
	// size Specifies the size in bytes of the buffer object's new data store.
	// data Specifies a pointer to data that will be copied into the data store for initialization, or NULL if no data is to be copied.
	// usage Specifies the expected usage pattern of the data store.
	//
	// glBufferData creates and initializes a buffer object's data store
	glBufferData(GL_ARRAY_BUFFER, size * sizeof(GLfloat), 0, GL_STATIC_DRAW);

	_vertexBuffers.push_back(vertexBuffer);

    checkForErrors();

	return vertexBuffer;
}

void
OpenGLES2Context::uploadVertexBufferData(const uint vertexBuffer,
									     const uint offset,
									     const uint size,
									     void* 				data)
{
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	// http://www.opengl.org/sdk/docs/man/xhtml/glBufferSubData.xml
	//
	// void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
	// target Specifies the target buffer object
	// offset Specifies the offset into the buffer object's data store where data replacement will begin, measured in bytes.
	// size Specifies the size in bytes of the data store region being replaced.
	// data Specifies a pointer to the new data that will be copied into the data store.
	//
	// glBufferSubData updates a subset of a buffer object's data store
	glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(GLfloat), size * sizeof(GLfloat), data);

    checkForErrors();
}

void
OpenGLES2Context::deleteVertexBuffer(const uint vertexBuffer)
{
	for (auto& currentVertexBuffer : _currentVertexBuffer)
		if (currentVertexBuffer == vertexBuffer)
			currentVertexBuffer = 0;

	_vertexBuffers.erase(std::find(_vertexBuffers.begin(), _vertexBuffers.end(), vertexBuffer));

	// http://www.opengl.org/sdk/docs/man/xhtml/glDeleteBuffers.xml
	//
	// void glDeleteBuffers(GLsizei n, const GLuint* buffers)
	// n Specifies the number of buffer objects to be deleted.
	// buffers Specifies an array of buffer objects to be deleted.
	//
	// glDeleteBuffers deletes n buffer objects named by the elements of the array buffers. After a buffer object is
	// deleted, it has no contents, and its name is free for reuse (for example by glGenBuffers). If a buffer object
	// that is currently bound is deleted, the binding reverts to 0 (the absence of any buffer object).
	glDeleteBuffers(1, &vertexBuffer);

    checkForErrors();
}

void
OpenGLES2Context::setVertexBufferAt(const uint	position,
								    const uint	vertexBuffer,
								    const uint	size,
								    const uint	stride,
								    const uint	offset)
{
	auto currentVertexBuffer = _currentVertexBuffer[position];

	if (currentVertexBuffer == vertexBuffer
		&& _currentVertexSize[position] == size
		&& _currentVertexStride[position] == stride
		&& _currentVertexOffset[position] == position)
		return ;

	_currentVertexBuffer[position] = vertexBuffer;
	_currentVertexSize[position] = size;
	_currentVertexStride[position] = stride;
	_currentVertexOffset[position] = offset;

	if (vertexBuffer > 0)
		glEnableVertexAttribArray(position);
	else
	{
		glDisableVertexAttribArray(position);
		
		return;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	// http://www.khronos.org/opengles/sdk/docs/man/xhtml/glVertexAttribPointer.xml
	glVertexAttribPointer(
		position,
		size,
		GL_FLOAT,
		GL_FALSE,
		sizeof(GLfloat) * stride,
		(void*)(sizeof(GLfloat) * offset)
	);

    checkForErrors();
}

const uint
OpenGLES2Context::createIndexBuffer(const uint size)
{
	uint indexBuffer;

	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	_currentIndexBuffer = indexBuffer;

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(GLushort), 0, GL_STATIC_DRAW);

	_indexBuffers.push_back(indexBuffer);

    checkForErrors();

	return indexBuffer;
}

void
OpenGLES2Context::uploaderIndexBufferData(const uint 	indexBuffer,
										  const uint 	offset,
										  const uint 	size,
										  void*					data)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

	_currentIndexBuffer = indexBuffer;
	
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(GLushort), size * sizeof(GLushort), data);

    checkForErrors();
}

void
OpenGLES2Context::deleteIndexBuffer(const uint indexBuffer)
{
	if (_currentIndexBuffer == indexBuffer)
		_currentIndexBuffer = 0;

	_indexBuffers.erase(std::find(_indexBuffers.begin(), _indexBuffers.end(), indexBuffer));

	glDeleteBuffers(1, &indexBuffer);

    checkForErrors();
}

const uint
OpenGLES2Context::createTexture(uint 	width,
							    uint 	height,
							    bool			mipMapping,
                                bool            optimizeForRenderToTexture)
{
	uint texture;

	// make sure width is a power of 2
	if (!((width != 0) && !(width & (width - 1))))
		throw std::invalid_argument("width");

	// make sure height is a power of 2
	if (!((height != 0) && !(height & (height - 1))))
		throw std::invalid_argument("height");

	// http://www.opengl.org/sdk/docs/man/xhtml/glGenTextures.xml
	//
	// void glGenTextures(GLsizei n, GLuint* textures)
	// n Specifies the number of texture names to be generated.
	// textures Specifies an array in which the generated texture names are stored.
	//
	// glGenTextures generate texture names
	glGenTextures(1, &texture);

	// http://www.opengl.org/sdk/docs/man/xhtml/glBindTexture.xml
	//
	// void glBindTexture(GLenum target, GLuint texture);
	// target Specifies the target to which the texture is bound.
	// texture Specifies the name of a texture.
	//
	// glBindTexture bind a named texture to a texturing target
	glBindTexture(GL_TEXTURE_2D, texture);

    // default sampler states
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  	_textures.push_back(texture);
    _textureSizes[texture] = std::make_pair(width, height);
    _textureHasMipmaps[texture] = mipMapping;
    _currentWrapMode[texture] = WrapMode::CLAMP;
    _currentTextureFilter[texture] = TextureFilter::NEAREST;
    _currentMipFilter[texture] = MipFilter::NONE;

	// http://www.opengl.org/sdk/docs/man/xhtml/glTexImage2D.xml
	//
	// void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border,
	// GLenum format, GLenum type, const GLvoid* data);
	// target Specifies the target texture.
	// level Specifies the level-of-detail number. Level 0 is the base image level. Level n is the nth mipmap reduction
	// image. If target is GL_TEXTURE_RECTANGLE or GL_PROXY_TEXTURE_RECTANGLE, level must be 0.
	// internalFormat Specifies the number of color components in the texture. Must be one of base internal formats given in Table 1,
	// one of the sized internal formats given in Table 2, or one of the compressed internal formats given in Table 3,
	// below.
	// width Specifies the width of the texture image.
	// height Specifies the height of the texture image.
	// border This value must be 0.
	// format Specifies the format of the pixel data.
	// type Specifies the data type of the pixel data
	// data Specifies a pointer to the image data in memory.
	//
	// glTexImage2D specify a two-dimensional texture image
	if (mipMapping)
    {
        uint level = 0;
        uint h = height;
        uint w = width;
		
		for (uint size = width > height ? width : height;
			 size > 0;
			 size = size >> 1, w = w >> 1, h = h >> 1)
		{
			glTexImage2D(GL_TEXTURE_2D, level++, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		}
    }
	else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    if (optimizeForRenderToTexture)
        createRTTBuffers(texture, width, height);

    checkForErrors();

	return texture;
}

void
OpenGLES2Context::uploadTextureData(const uint 	texture,
								    uint 		width,
								    uint 		height,
								    uint 		mipLevel,
								    void*				data)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, mipLevel, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    checkForErrors();
}

void
OpenGLES2Context::deleteTexture(const uint texture)
{
	_textures.erase(std::find(_textures.begin(), _textures.end(), texture));

	glDeleteTextures(1, &texture);

    if (_frameBuffers.count(texture))
    {
        glDeleteFramebuffers(1, &_frameBuffers[texture]);
        _frameBuffers.erase(texture);

        glDeleteRenderbuffers(1, &_renderBuffers[texture]);
        _renderBuffers.erase(texture);
    }

    _textureSizes.erase(texture);
    _textureHasMipmaps.erase(texture);
    _currentWrapMode.erase(texture);
    _currentTextureFilter.erase(texture);
    _currentMipFilter.erase(texture);

    checkForErrors();
}

void
OpenGLES2Context::setTextureAt(const uint	position,
							   const int	texture,
							   const int	location)
{
	auto textureIsValid = texture > 0;

    if (_currentTexture[position] != texture)
	{
		_currentTexture[position] = texture;

		glActiveTexture(GL_TEXTURE0 + position);
		glBindTexture(GL_TEXTURE_2D, texture);
	}

	if (textureIsValid && location >= 0)
		glUniform1i(location, position);

    checkForErrors();
}

void
OpenGLES2Context::setSamplerStateAt(const uint position, WrapMode wrapping, TextureFilter filtering, MipFilter mipFiltering)
{
    auto texture    = _currentTexture[position];
    auto active     = false;

    // disable mip mapping if mip maps are not available
    if (!_textureHasMipmaps[_currentTexture[position]])
        mipFiltering = MipFilter::NONE;

    if (_currentWrapMode[texture] != wrapping)
    {
        _currentWrapMode[texture] = wrapping;

        glActiveTexture(GL_TEXTURE0 + position);
        active = true;

        switch (wrapping)
        {
        case WrapMode::CLAMP :
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            break;
        case WrapMode::REPEAT :
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            break;
        }
    }
    
    if (_currentTextureFilter[texture] != filtering || _currentMipFilter[texture] != mipFiltering)
    {
        _currentTextureFilter[texture] = filtering;
        _currentMipFilter[texture] = mipFiltering;

        if (!active)
            glActiveTexture(GL_TEXTURE0 + position);

        switch (filtering)
        {
        case TextureFilter::NEAREST :
            switch (mipFiltering)
            {
            case MipFilter::NONE :
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                break;
            case MipFilter::NEAREST :
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
                break;
            case MipFilter::LINEAR :
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                break;
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case TextureFilter::LINEAR :
            switch (mipFiltering)
            {
            case MipFilter::NONE :
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                break;
            case MipFilter::NEAREST :
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                break;
            case MipFilter::LINEAR :
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                break;
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        }
    }

    checkForErrors();
}

const uint
OpenGLES2Context::createProgram()
{
	auto handle = glCreateProgram();
	
	checkForErrors();
	_programs.push_back(handle);

	return handle;
}

void
OpenGLES2Context::attachShader(const uint program, const uint shader)
{
	glAttachShader(program, shader);

    checkForErrors();
}

void
OpenGLES2Context::linkProgram(const uint program)
{
	glLinkProgram(program);

#ifdef DEBUG
    auto errors = getProgramInfoLogs(program);

    if (!errors.empty())
	{
    	std::cout << errors << std::endl;
	}
#endif

    checkForErrors();
}

void
OpenGLES2Context::deleteProgram(const uint program)
{
	_programs.erase(std::find(_programs.begin(), _programs.end(), program));

	glDeleteProgram(program);

    checkForErrors();
}

void
OpenGLES2Context::compileShader(const uint shader)
{
	glCompileShader(shader);

#ifdef DEBUG
    auto errors = getShaderCompilationLogs(shader);

    if (!errors.empty())
    {
		std::stringstream	stream;
		stream << "glShaderSource_" << shader << ".txt";

		const std::string&	filename	= stream.str();

		saveShaderSourceToFile(filename, shader);

		std::cerr << "\nERRORS\n------\n" << errors << std::endl;
		std::cerr << "\nerrorneous shader source saved to \'" << filename << "\'" << std::endl;
		throw;
	}
#endif

    checkForErrors();
}

void
OpenGLES2Context::setProgram(const uint program)
{
	if (_currentProgram == program)
		return;

	_currentProgram = program;

	glUseProgram(program);

    checkForErrors();
}

void
OpenGLES2Context::setShaderSource(const uint shader,
							      const std::string& source)
{
#ifdef MINKO_GLSL_OPTIMIZER
	glslopt_ctx* glslOptimizer = glslopt_initialize(true);
    std::string src = "#version 100\n" + source;
	const char* sourceString = src.c_str();

    auto type = std::find(_vertexShaders.begin(), _vertexShaders.end(), shader) != _vertexShaders.end()
        ? kGlslOptShaderVertex
        : kGlslOptShaderFragment;

    auto optimizedShader = glslopt_optimize(glslOptimizer, type, sourceString, 0);
    if (glslopt_get_status(optimizedShader))
    {
        auto optimizedSource = glslopt_get_output(optimizedShader);
        glShaderSource(shader, 1, &optimizedSource, 0);
    }
    else
    {
		std::stringstream stream(source);
		std::string line;

        std::cerr << glslopt_get_log(optimizedShader) << std::endl;
		for (auto i = 0; std::getline(stream, line); ++i)
			std::cerr << i << "\t" << line << std::endl;

        throw std::invalid_argument("source");
    }
    glslopt_shader_delete(optimizedShader);
    glslopt_cleanup(glslOptimizer);
#else
# ifdef GL_ES_VERSION_2_0
	std::string src = "#version 100\n" + source;
# else
    std::string src = "#version 120\n" + source;
#endif
	const char* sourceString = src.c_str();

    glShaderSource(shader, 1, &sourceString, 0);
#endif

    checkForErrors();
}

void
OpenGLES2Context::saveShaderSourceToFile(const std::string& filename, uint shader)
{
	std::string	source;
	getShaderSource(shader, source);

#ifndef MINKO_GLSL_OPTIMIZER
	std::cout << "\nSHADER SOURCE\n-------------" << std::endl;
	unsigned int i		= 0;
	unsigned int line	= 1;
	while(i < source.size())
	{
		std::string lineString;
		while(i < source.size() && source[i] != '\n')
			lineString.push_back(source[i++]);
		++i;

#ifndef EMSCRIPTEN
		std::cerr
#else
		std::cout
#endif // EMSCRIPTEN
			<< "(" << std::setfill('0') << std::setw(4) << line << ") " << lineString << std::endl;

		++line;
	}
#endif //MINKO_GLSL_OPTIMIZER

#ifndef EMSCRIPTEN
	std::ofstream	file;

	file.open(filename.c_str());
	if (!file.is_open())
		return;
	file << source;
	file.close();
#endif // EMSCRIPTEN
}

void
OpenGLES2Context::getShaderSource(uint shader, std::string& source)
{
	source.clear();

	GLint	bufferSize	= 0;
	GLsizei	length		= 0;

	glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &bufferSize);
	if (bufferSize == 0)
		return;

	source.resize(bufferSize);
	glGetShaderSource(shader, bufferSize, &length, &source[0]);
	checkForErrors();
}


const uint
OpenGLES2Context::createVertexShader()
{
	uint vertexShader = glCreateShader(GL_VERTEX_SHADER);

	_vertexShaders.push_back(vertexShader);

	checkForErrors();

	return vertexShader;
}

void
OpenGLES2Context::deleteVertexShader(const uint vertexShader)
{
	_vertexShaders.erase(std::find(_vertexShaders.begin(), _vertexShaders.end(), vertexShader));

	glDeleteShader(vertexShader);

    checkForErrors();
}

const uint
OpenGLES2Context::createFragmentShader()
{
	uint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	_fragmentShaders.push_back(fragmentShader);

	checkForErrors();

	return fragmentShader;
}

void
OpenGLES2Context::deleteFragmentShader(const uint fragmentShader)
{
	_fragmentShaders.erase(std::find(_fragmentShaders.begin(), _fragmentShaders.end(), fragmentShader));

	glDeleteShader(fragmentShader);

    checkForErrors();
}

std::shared_ptr<ProgramInputs>
OpenGLES2Context::getProgramInputs(const uint program)
{
	std::vector<std::string> names;
	std::vector<ProgramInputs::Type> types;
	std::vector<uint> locations;

	glUseProgram(program);
	fillUniformInputs(program, names, types, locations);
	fillAttributeInputs(program, names, types, locations);

	return ProgramInputs::create(shared_from_this(), program, names, types, locations);
}

void
OpenGLES2Context::fillUniformInputs(const uint					program,
								    std::vector<std::string>&			names,
								    std::vector<ProgramInputs::Type>&	types,
								    std::vector<uint>&			locations)
{
	int total = -1;
	int maxUniformNameLength = -1;

	glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLength);
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &total);

	for (int i = 0; i < total; ++i)
	{
    	int nameLength = -1;
    	int size = -1;
    	GLenum type = GL_ZERO;
    	std::vector<char> name(maxUniformNameLength);

    	glGetActiveUniform(program, i, maxUniformNameLength, &nameLength, &size, &type, &name[0]);
    	checkForErrors();

	    name[nameLength] = 0;

	    ProgramInputs::Type inputType = ProgramInputs::Type::unknown;

	    switch (type)
	    {
	    	case GL_FLOAT:
	    		inputType = ProgramInputs::Type::float1;
	    		break;
	    	case GL_INT:
	    		inputType = ProgramInputs::Type::int1;
	    		break;
	    	case GL_FLOAT_VEC2:
	    		inputType = ProgramInputs::Type::float2;
	    		break;
	    	case GL_INT_VEC2:
	    		inputType = ProgramInputs::Type::int2;
		    	break;
	    	case GL_FLOAT_VEC3:
	    		inputType = ProgramInputs::Type::float3;
	    		break;
	    	case GL_INT_VEC3:
	    		inputType = ProgramInputs::Type::int3;
	    		break;
	    	case GL_FLOAT_VEC4:
	    		inputType = ProgramInputs::Type::float4;
	    		break;
	    	case GL_INT_VEC4:
	    		inputType = ProgramInputs::Type::int4;
	    		break;
	    	case GL_FLOAT_MAT3:
	    		inputType = ProgramInputs::Type::float9;
		    	break;
	    	case GL_FLOAT_MAT4:
	    		inputType = ProgramInputs::Type::float16;
	    		break;
			case GL_SAMPLER_2D:
				inputType = ProgramInputs::Type::sampler2d;
				break;
			default:
				throw std::logic_error("unsupported type");
	    }

	    int location = glGetUniformLocation(program, &name[0]);

	    if (location >= 0 && inputType != ProgramInputs::Type::unknown)
	    {
		    names.push_back(std::string(&name[0], nameLength));
		    types.push_back(inputType);
		    locations.push_back(location);
		}
	}
}

void
OpenGLES2Context::fillAttributeInputs(const uint				program,
									 std::vector<std::string>&			names,
								     std::vector<ProgramInputs::Type>&	types,
								     std::vector<uint>&			locations)
{
	int total = -1;
	int maxAttributeNameLength = -1;

	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttributeNameLength);
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &total);

	for (int i = 0; i < total; ++i)
	{
    	int nameLength = -1;
    	int size = -1;
    	GLenum type = GL_ZERO;
    	std::vector<char> name(maxAttributeNameLength);

		glGetActiveAttrib(program, i, maxAttributeNameLength, &nameLength, &size, &type, &name[0]);
	    checkForErrors();

	    name[nameLength] = 0;

	    ProgramInputs::Type inputType = ProgramInputs::Type::attribute;

		int location = glGetAttribLocation(program, &name[0]);

	    if (location >= 0)
	    {
		    names.push_back(std::string(&name[0], nameLength));
		    types.push_back(inputType);
		    locations.push_back(location);
		}
	}
}

std::string
OpenGLES2Context::getShaderCompilationLogs(const uint shader)
{
	int compileStatus = -1;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (compileStatus != GL_TRUE)
	{
		int logsLength = -1;
		char buffer[1024];
		int bufferLength = -1;

		glGetShaderSource(shader, 1024, &bufferLength, buffer);

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logsLength);

		if (logsLength > 0)
		{
			std::vector<char> logs(logsLength);

			glGetShaderInfoLog(shader, logsLength, &logsLength, &logs[0]);

			return std::string(&logs[0]);
		}
	}

	return std::string();
}

std::string
OpenGLES2Context::getProgramInfoLogs(const uint program)
{
	int programInfoMaxLength = -1;
	int programInfoLength = -1;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &programInfoMaxLength);

	if (programInfoMaxLength <= 0)
		return std::string();

	std::vector<char> programInfo(programInfoMaxLength);

	glGetProgramInfoLog(program, programInfoMaxLength, &programInfoLength, &programInfo[0]);

	return std::string(&programInfo[0]);
}

void
OpenGLES2Context::setUniform(const uint& location, const int& value)
{
	glUniform1i(location, value);
	checkForErrors();
}

void
OpenGLES2Context::setUniform(const uint& location, const int& v1, const int& v2)
{
	glUniform2i(location, v1, v2);
	checkForErrors();
}

void
OpenGLES2Context::setUniform(const uint& location, const int& v1, const int& v2, const int& v3)
{
	glUniform3i(location, v1, v2, v3);
	checkForErrors();
}

void
OpenGLES2Context::setUniform(const uint& location, const int& v1, const int& v2, const int& v3, const int& v4)
{
	glUniform4i(location, v1, v2, v3, v4);
	checkForErrors();
}

void
OpenGLES2Context::setUniform(const uint& location, const float& value)
{
	glUniform1f(location, value);
	checkForErrors();
}

void
OpenGLES2Context::setUniform(const uint& location, const float& v1, const float& v2)
{
	glUniform2f(location, v1, v2);
	checkForErrors();
}

void
OpenGLES2Context::setUniform(const uint& location, const float& v1, const float& v2, const float& v3)
{
	glUniform3f(location, v1, v2, v3);
	checkForErrors();
}

void
OpenGLES2Context::setUniform(const uint& location, const float& v1, const float& v2, const float& v3, const float& v4)
{
	glUniform4f(location, v1, v2, v3, v4);
	checkForErrors();
}

void
OpenGLES2Context::setUniforms(uint location, uint size, const float* values)
{
	glUniform1fv(location, size, values);
	checkForErrors();
}

void
OpenGLES2Context::setUniforms2(uint location, uint size, const float* values)
{
	glUniform2fv(location, size, values);
	checkForErrors();
}

void
OpenGLES2Context::setUniforms3(uint location, uint size, const float* values)
{
	glUniform3fv(location, size, values);
	checkForErrors();
}

void
OpenGLES2Context::setUniforms4(uint location, uint size, const float* values)
{
	glUniform4fv(location, size, values);
	checkForErrors();
}

void
OpenGLES2Context::setUniform(const uint& location, const uint& size, bool transpose, const float* values)
{
#ifdef GL_ES_VERSION_2_0
    if (transpose)
    {
		float tmp[16];
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				tmp[i * 4 + j] = values[j * 4 + i];

        glUniformMatrix4fv(location, size, false, tmp);
    }
    else
    {
        glUniformMatrix4fv(location, size, transpose, values);
    }
#else

	glUniformMatrix4fv(location, size, transpose, values);
#endif
	checkForErrors();
}

void
OpenGLES2Context::setBlendMode(Blending::Source source, Blending::Destination destination)
{
	if ((static_cast<uint>(source) | static_cast<uint>(destination)) != static_cast<uint>(_currentBlendMode))
	{
		_currentBlendMode = (Blending::Mode)((uint)source | (uint)destination);

		glBlendFunc(
			_blendingFactors[static_cast<uint>(source) & 0x00ff],
			_blendingFactors[static_cast<uint>(destination) & 0xff00]
		);
	}

    checkForErrors();
}

void
OpenGLES2Context::setBlendMode(Blending::Mode blendMode)
{
	if (blendMode != _currentBlendMode)
	{
		_currentBlendMode = blendMode;

		glBlendFunc(
			_blendingFactors[static_cast<uint>(blendMode) & 0x00ff],
			_blendingFactors[static_cast<uint>(blendMode) & 0xff00]
		);
	}

    checkForErrors();
}

void
OpenGLES2Context::setDepthTest(bool depthMask, CompareMode depthFunc)
{
	if (depthMask != _currentDepthMask || depthFunc != _currentDepthFunc)
	{
		_currentDepthMask = depthMask;
		_currentDepthFunc = depthFunc;

		glDepthMask(depthMask);
		glDepthFunc(_compareFuncs[depthFunc]);
	}

    checkForErrors();
}

void
OpenGLES2Context::setColorMask(bool colorMask)
{
	if (_currentColorMask != colorMask)
	{
		_currentColorMask = colorMask;

		glColorMask(colorMask, colorMask, colorMask, colorMask);
	}

	checkForErrors();
}

void
OpenGLES2Context::setStencilTest(CompareMode stencilFunc, 
								 int stencilRef, 
								 uint stencilMask, 
								 StencilOperation stencilFailOp,
								 StencilOperation stencilZFailOp,
								 StencilOperation stencilZPassOp)
{
#ifndef MINKO_NO_STENCIL
	if (stencilFunc != _currentStencilFunc 
		|| stencilRef != _currentStencilRef 
		|| stencilMask != _currentStencilMask)
	{
		_currentStencilFunc	= stencilFunc;
		_currentStencilRef	= stencilRef;
		_currentStencilMask	= stencilMask;

		glStencilFunc(_compareFuncs[stencilFunc], stencilRef, stencilMask);
	}

	checkForErrors();

	if (stencilFailOp != _currentStencilFailOp
		|| stencilZFailOp != _currentStencilZFailOp
		|| stencilZPassOp != _currentStencilZPassOp)
	{
		_currentStencilFailOp	= stencilFailOp;
		_currentStencilZFailOp	= stencilZFailOp;
		_currentStencilZPassOp	= stencilZPassOp;

		glStencilOp(_stencilOps[stencilFailOp], _stencilOps[stencilZFailOp], _stencilOps[stencilZPassOp]);
	}

	checkForErrors();
#endif
}

void
OpenGLES2Context::readPixels(unsigned char* pixels)
{
	glReadPixels(_viewportX, _viewportY, _viewportWidth, _viewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    checkForErrors();
}

void
OpenGLES2Context::setTriangleCulling(TriangleCulling triangleCulling)
{
    if (triangleCulling == _currentTriangleCulling)
        return;

    if (_currentTriangleCulling == TriangleCulling::NONE)
        glEnable(GL_CULL_FACE);
    _currentTriangleCulling = triangleCulling;

    switch (triangleCulling)
    {
    case TriangleCulling::NONE:
        glDisable(GL_CULL_FACE);
        break;
    case TriangleCulling::BACK :
        glCullFace(GL_BACK);
        break;
    case TriangleCulling::FRONT :
        glCullFace(GL_FRONT);
        break;
    case TriangleCulling::BOTH :
        glCullFace(GL_FRONT_AND_BACK);
        break;
    }

    checkForErrors();
}

void
OpenGLES2Context::setRenderToBackBuffer()
{
    if (_currentTarget == 0)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glViewport(_viewportX, _viewportY, _viewportWidth, _viewportHeight);

    _currentTarget = 0;

    checkForErrors();
}

void
OpenGLES2Context::setRenderToTexture(uint texture, bool enableDepthAndStencil)
{
    if (texture == _currentTarget)
        return;

    if (_frameBuffers.count(texture) == 0)
        throw std::logic_error("this texture cannot be used for RTT");

    _currentTarget = texture;

	glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffers[texture]);
	checkForErrors();

    if (enableDepthAndStencil)
		glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffers[texture]);
	checkForErrors();

    auto textureSize = _textureSizes[texture];

    glViewport(0, 0, textureSize.first, textureSize.second);

    checkForErrors();
}

void
OpenGLES2Context::createRTTBuffers(uint texture, uint width, uint height)
{
    uint frameBuffer = -1;

    // create a framebuffer object
    glGenFramebuffers(1, &frameBuffer);
    // bind the framebuffer object
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    // attach a texture to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    uint renderBuffer = -1;

    // gen renderbuffer
    glGenRenderbuffers(1, &renderBuffer);
    // bind renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
    // init as a depth buffer
#ifdef GL_ES_VERSION_2_0
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
#else
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
#endif
	// FIXME: create & attach stencil buffer

    // attach to the FBO for depth
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        throw;

    _frameBuffers[texture] = frameBuffer;
    _renderBuffers[texture] = renderBuffer;

    // unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    checkForErrors();
}

uint
OpenGLES2Context::getError()
{
	auto error = glGetError();

	switch(error)
	{
	default:
		break;
	case GL_INVALID_ENUM:
		std::cerr << "GL_INVALID_ENUM" << std::endl;
		break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION" << std::endl;
		break;
	case GL_INVALID_VALUE:
		std::cerr << "GL_INVALID_VALUE" << std::endl;
		break;
	case GL_INVALID_OPERATION:
		std::cerr << "GL_INVALID_OPERATION" << std::endl;
		break;
	case GL_OUT_OF_MEMORY:
		std::cerr << "GL_OUT_OF_MEMORY" << std::endl;
		break;
	}

    return error;
}

void
OpenGLES2Context::generateMipmaps(uint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glGenerateMipmap(GL_TEXTURE_2D);

    checkForErrors();
}
