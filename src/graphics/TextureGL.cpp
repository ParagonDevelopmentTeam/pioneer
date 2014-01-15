// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "TextureGL.h"
#include <cassert>
#include "utils.h"

static const unsigned int MIN_COMPRESSED_TEXTURE_DIMENSION = 16;

namespace Graphics {

inline GLint GLInternalFormat(TextureFormat format) {
	switch (format) {
		case TEXTURE_RGB_888: return GL_RGB;
		case TEXTURE_RGBA_8888: return GL_RGBA;
		case TEXTURE_LUMINANCE_ALPHA_88: return GL_LUMINANCE_ALPHA;
		case TEXTURE_INTENSITY_8:  return GL_INTENSITY;
		case TEXTURE_DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case TEXTURE_DXT1:  return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case TEXTURE_DEPTH: return GL_DEPTH_COMPONENT;
		default: assert(0); return 0;
	}
}

//for on the fly compression of textures
//luminance/intensity is used for fonts, so we prefer not to compress them
inline GLint GLCompressedInternalFormat(TextureFormat format) {
	switch (format) {
		case TEXTURE_RGBA_8888: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case TEXTURE_RGB_888:  return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case TEXTURE_LUMINANCE_ALPHA_88: return GL_LUMINANCE_ALPHA;
		case TEXTURE_INTENSITY_8:  return GL_INTENSITY;
		case TEXTURE_DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case TEXTURE_DXT1:  return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		default: assert(0); return 0;
	}
}

inline GLint GLImageFormat(TextureFormat format) {
	switch (format) {
		case TEXTURE_RGBA_8888: return GL_RGBA;
		case TEXTURE_RGB_888:  return GL_RGB;
		case TEXTURE_LUMINANCE_ALPHA_88: return GL_LUMINANCE_ALPHA;
		case TEXTURE_INTENSITY_8:  return GL_LUMINANCE;
		case TEXTURE_DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case TEXTURE_DXT1:  return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case TEXTURE_DEPTH: return GL_DEPTH_COMPONENT;
		default: assert(0); return 0;
	}
}

inline GLint GLTextureType(TextureType type) {
	switch (type) {
		case TEXTURE_2D: return GL_TEXTURE_2D;
		case TEXTURE_CUBE_MAP: return GL_TEXTURE_CUBE_MAP;
		default: assert(0); return 0;
	}
}

inline GLint GLImageType(TextureFormat format) {
	return GL_UNSIGNED_BYTE;
}

inline int GetMinSize(TextureFormat flag) {
	switch(flag) {
	case TEXTURE_DXT1: return 8;
	case TEXTURE_DXT5: return 16;
	default: return 1;
	}
}

inline bool IsCompressed(TextureFormat format) {
	return (format == TEXTURE_DXT1 || format == TEXTURE_DXT5);
}

TextureGL::TextureGL(const TextureDescriptor &descriptor, const bool useCompressed) :
	Texture(descriptor)
{
	m_target = GLTextureType(descriptor.type);

	glGenTextures(1, &m_texture);
	glBindTexture(m_target, m_texture);


	// useCompressed is the global scope flag whereas descriptor.allowCompression is the local texture mode flag
	// either both or neither might be true however only compress the texture when both are true.
	const bool compressTexture = useCompressed && descriptor.allowCompression;

	switch (m_target) {
		case GL_TEXTURE_2D:
			if (!IsCompressed(descriptor.format)) {
				if (descriptor.generateMipmaps)
					glTexParameteri(m_target, GL_GENERATE_MIPMAP, GL_TRUE);
				else
					glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, 0);

				glTexImage2D(
					m_target, 0, compressTexture ? GLCompressedInternalFormat(descriptor.format) : GLInternalFormat(descriptor.format),
					descriptor.dataSize.x, descriptor.dataSize.y, 0,
					GLImageFormat(descriptor.format),
					GLImageType(descriptor.format), 0);
			} else {
				const GLint oglFormatMinSize = GetMinSize(descriptor.format);
				size_t Width = descriptor.dataSize.x;
				size_t Height = descriptor.dataSize.y;
				size_t bufSize = ((Width + 3) / 4) * ((Height + 3) / 4) * oglFormatMinSize;

				GLint maxMip = 0;
				for( unsigned int i=0; i < descriptor.numberOfMipMaps; ++i ) {
					maxMip = i;
					glCompressedTexImage2D(GL_TEXTURE_2D, i, GLInternalFormat(descriptor.format), Width, Height, 0, bufSize, 0);
					if( Width<=MIN_COMPRESSED_TEXTURE_DIMENSION || Height<=MIN_COMPRESSED_TEXTURE_DIMENSION ) {
						break;
					}
					bufSize /= 4;
					Width /= 2;
					Height /= 2;
				}
				glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, maxMip);
			}
			break;

		case GL_TEXTURE_CUBE_MAP:
			if(!IsCompressed(descriptor.format)) {
				if(descriptor.generateMipmaps)
					glTexParameteri(m_target, GL_GENERATE_MIPMAP, GL_TRUE);
				else
					glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, 0);

				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, compressTexture ? GLCompressedInternalFormat(descriptor.format) : GLInternalFormat(descriptor.format),
					descriptor.dataSize.x, descriptor.dataSize.y, 0,
					GLImageFormat(descriptor.format),
					GLImageType(descriptor.format), 0);
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, compressTexture ? GLCompressedInternalFormat(descriptor.format) : GLInternalFormat(descriptor.format),
					descriptor.dataSize.x, descriptor.dataSize.y, 0,
					GLImageFormat(descriptor.format),
					GLImageType(descriptor.format), 0);
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, compressTexture ? GLCompressedInternalFormat(descriptor.format) : GLInternalFormat(descriptor.format),
					descriptor.dataSize.x, descriptor.dataSize.y, 0,
					GLImageFormat(descriptor.format),
					GLImageType(descriptor.format), 0);
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, compressTexture ? GLCompressedInternalFormat(descriptor.format) : GLInternalFormat(descriptor.format),
					descriptor.dataSize.x, descriptor.dataSize.y, 0,
					GLImageFormat(descriptor.format),
					GLImageType(descriptor.format), 0);
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, compressTexture ? GLCompressedInternalFormat(descriptor.format) : GLInternalFormat(descriptor.format),
					descriptor.dataSize.x, descriptor.dataSize.y, 0,
					GLImageFormat(descriptor.format),
					GLImageType(descriptor.format), 0);
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, compressTexture ? GLCompressedInternalFormat(descriptor.format) : GLInternalFormat(descriptor.format),
					descriptor.dataSize.x, descriptor.dataSize.y, 0,
					GLImageFormat(descriptor.format),
					GLImageType(descriptor.format), 0);
			} else {
				const GLint oglFormatMinSize = GetMinSize(descriptor.format);
				size_t Width = descriptor.dataSize.x;
				size_t Height = descriptor.dataSize.y;
				size_t bufSize = ((Width + 3) / 4) * ((Height + 3) / 4) * oglFormatMinSize;

				GLint maxMip = 0;
				for( unsigned int i=0; i < descriptor.numberOfMipMaps; ++i ) {
					maxMip = i;
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, i, GLInternalFormat(descriptor.format), Width, Height, 0, bufSize, 0);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, i, GLInternalFormat(descriptor.format), Width, Height, 0, bufSize, 0);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, i, GLInternalFormat(descriptor.format), Width, Height, 0, bufSize, 0);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, i, GLInternalFormat(descriptor.format), Width, Height, 0, bufSize, 0);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, i, GLInternalFormat(descriptor.format), Width, Height, 0, bufSize, 0);
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, i, GLInternalFormat(descriptor.format), Width, Height, 0, bufSize, 0);
					if( Width<=MIN_COMPRESSED_TEXTURE_DIMENSION || Height<=MIN_COMPRESSED_TEXTURE_DIMENSION ) {
						break;
					}
					bufSize /= 4;
					Width /= 2;
					Height /= 2;
				}
				glTexParameteri(m_target, GL_TEXTURE_MAX_LEVEL, maxMip);
			}
			break;

		default:
			assert(0);
	}

	GLenum magFilter, minFilter, wrapS, wrapT;
	switch (descriptor.sampleMode) {
		case LINEAR_CLAMP:
			magFilter = GL_LINEAR;
			minFilter = descriptor.generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
			wrapS = wrapT = GL_CLAMP_TO_EDGE;
			break;

		case NEAREST_CLAMP:
			magFilter = GL_NEAREST;
			minFilter = descriptor.generateMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
			wrapS = wrapT = GL_CLAMP_TO_EDGE;
			break;

		case LINEAR_REPEAT:
			magFilter = GL_LINEAR;
			minFilter = descriptor.generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
			wrapS = wrapT = GL_REPEAT;
			break;

		case NEAREST_REPEAT:
			magFilter = GL_NEAREST;
			minFilter = descriptor.generateMipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
			wrapS = wrapT = GL_REPEAT;
			break;

		default:
			assert(0);
	}

	glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrapS);
	glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, minFilter);

}

TextureGL::~TextureGL()
{
	glDeleteTextures(1, &m_texture);
}

void TextureGL::Update(const void *data, const vector2f &dataSize, TextureFormat format, const unsigned int numMips)
{
	assert(m_target == GL_TEXTURE_2D);

	glBindTexture(m_target, m_texture);

	switch (m_target) {
		case GL_TEXTURE_2D:
			if (!IsCompressed(format)) {
				glTexSubImage2D(m_target, 0, 0, 0, dataSize.x, dataSize.y, GLImageFormat(format), GLImageType(format), data);
			} else {
				const GLint oglInternalFormat = GLImageFormat(format);
				size_t Offset = 0;
				size_t Width = dataSize.x;
				size_t Height = dataSize.y;
				size_t bufSize = ((Width + 3) / 4) * ((Height + 3) / 4) * GetMinSize(format);

				const unsigned char *pData = static_cast<const unsigned char*>(data);
				for( unsigned int i = 0; i < numMips; ++i ) {
					glCompressedTexSubImage2D(m_target, i, 0, 0, Width, Height, oglInternalFormat, bufSize, &pData[Offset]);
					if( Width<=MIN_COMPRESSED_TEXTURE_DIMENSION || Height<=MIN_COMPRESSED_TEXTURE_DIMENSION ) {
						break;
					}
					Offset += bufSize;
					bufSize /= 4;
					Width /= 2;
					Height /= 2;
				}
			}
			break;

		default:
			assert(0);
	}

	glBindTexture(m_target, 0);
}

void TextureGL::Update(const TextureCubeData &data, const vector2f &dataSize, TextureFormat format, const unsigned int numMips)
{
	assert(m_target == GL_TEXTURE_CUBE_MAP);

	glBindTexture(m_target, m_texture);

	switch (m_target) {
		case GL_TEXTURE_CUBE_MAP:
			if (!IsCompressed(format)) {
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, dataSize.x, dataSize.y, GLImageFormat(format), GLImageType(format), data.posX);
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 0, 0, dataSize.x, dataSize.y, GLImageFormat(format), GLImageType(format), data.negX);
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, 0, 0, dataSize.x, dataSize.y, GLImageFormat(format), GLImageType(format), data.posY);
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, 0, 0, dataSize.x, dataSize.y, GLImageFormat(format), GLImageType(format), data.negY);
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, 0, 0, dataSize.x, dataSize.y, GLImageFormat(format), GLImageType(format), data.posZ);
				glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 0, 0, dataSize.x, dataSize.y, GLImageFormat(format), GLImageType(format), data.negZ);
			} else {
				const GLint oglInternalFormat = GLImageFormat(format);
				size_t Offset = 0;
				size_t Width = dataSize.x;
				size_t Height = dataSize.y;
				size_t bufSize = ((Width + 3) / 4) * ((Height + 3) / 4) * GetMinSize(format);

				const unsigned char *pData_px = static_cast<const unsigned char*>(data.posX);
				const unsigned char *pData_nx = static_cast<const unsigned char*>(data.negX);
				const unsigned char *pData_py = static_cast<const unsigned char*>(data.posY);
				const unsigned char *pData_ny = static_cast<const unsigned char*>(data.negY);
				const unsigned char *pData_pz = static_cast<const unsigned char*>(data.posZ);
				const unsigned char *pData_nz = static_cast<const unsigned char*>(data.negZ);
				for( unsigned int i = 0; i < numMips; ++i ) {
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, i, 0, 0, Width, Height, oglInternalFormat, bufSize, &pData_px[Offset]);
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, i, 0, 0, Width, Height, oglInternalFormat, bufSize, &pData_nx[Offset]);
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, i, 0, 0, Width, Height, oglInternalFormat, bufSize, &pData_py[Offset]);
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, i, 0, 0, Width, Height, oglInternalFormat, bufSize, &pData_ny[Offset]);
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, i, 0, 0, Width, Height, oglInternalFormat, bufSize, &pData_pz[Offset]);
					glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, i, 0, 0, Width, Height, oglInternalFormat, bufSize, &pData_nz[Offset]);
					if( Width<=MIN_COMPRESSED_TEXTURE_DIMENSION || Height<=MIN_COMPRESSED_TEXTURE_DIMENSION ) {
						break;
					}
					Offset += bufSize;
					bufSize /= 4;
					Width /= 2;
					Height /= 2;
				}
			}
			break;

		default:
			assert(0);
	}

	glBindTexture(m_target, 0);
}

void TextureGL::Bind()
{
	glBindTexture(m_target, m_texture);
}

void TextureGL::Unbind()
{
	glBindTexture(m_target, 0);
}

void TextureGL::SetSampleMode(TextureSampleMode mode)
{
	GLenum magFilter, minFilter;
	const bool mipmaps = GetDescriptor().generateMipmaps;
	switch (mode) {
		case LINEAR_CLAMP:
			magFilter = GL_LINEAR;
			minFilter = mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
			break;

		case NEAREST_CLAMP:
			magFilter = GL_NEAREST;
			minFilter = mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
			break;

		case LINEAR_REPEAT:
			magFilter = GL_LINEAR;
			minFilter = mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
			break;

		case NEAREST_REPEAT:
			magFilter = GL_NEAREST;
			minFilter =mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
			break;

		default:
			assert(0);
	}
	glBindTexture(m_target, m_texture);
	glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, minFilter);
	glBindTexture(m_target, 0);
}

}
