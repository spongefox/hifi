//
//  Texture.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/16/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Texture_h
#define hifi_gpu_Texture_h

#include <algorithm> //min max and more
#include <bitset>

#include <QMetaType>
#include <QUrl>

#include <shared/Storage.h>

#include "Forward.h"
#include "Resource.h"

namespace ktx {
    class KTX;
    using KTXUniquePointer = std::unique_ptr<KTX>;
    struct KTXDescriptor;
    using KTXDescriptorPointer = std::unique_ptr<KTXDescriptor>;
    struct Header;
}

namespace gpu {

// THe spherical harmonics is a nice tool for cubemap, so if required, the irradiance SH can be automatically generated
// with the cube texture
class Texture;
class SphericalHarmonics {
public:
    glm::vec3 L00    ; float spare0;
    glm::vec3 L1m1   ; float spare1;
    glm::vec3 L10    ; float spare2;
    glm::vec3 L11    ; float spare3;
    glm::vec3 L2m2   ; float spare4;
    glm::vec3 L2m1   ; float spare5;
    glm::vec3 L20    ; float spare6;
    glm::vec3 L21    ; float spare7;
    glm::vec3 L22    ; float spare8;

    static const int NUM_COEFFICIENTS = 9;

    enum Preset {
        OLD_TOWN_SQUARE = 0,
        GRACE_CATHEDRAL,
        EUCALYPTUS_GROVE,
        ST_PETERS_BASILICA,
        UFFIZI_GALLERY,
        GALILEOS_TOMB,
        VINE_STREET_KITCHEN,
        BREEZEWAY,
        CAMPUS_SUNSET,
        FUNSTON_BEACH_SUNSET,

        NUM_PRESET,
    };

    void assignPreset(int p);

    void evalFromTexture(const Texture& texture);
};
typedef std::shared_ptr< SphericalHarmonics > SHPointer;

class Sampler {
public:

    enum Filter {
        FILTER_MIN_MAG_POINT, // top mip only
        FILTER_MIN_POINT_MAG_LINEAR, // top mip only
        FILTER_MIN_LINEAR_MAG_POINT, // top mip only
        FILTER_MIN_MAG_LINEAR, // top mip only

        FILTER_MIN_MAG_MIP_POINT,
        FILTER_MIN_MAG_POINT_MIP_LINEAR,
        FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
        FILTER_MIN_POINT_MAG_MIP_LINEAR,
        FILTER_MIN_LINEAR_MAG_MIP_POINT,
        FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        FILTER_MIN_MAG_LINEAR_MIP_POINT,
        FILTER_MIN_MAG_MIP_LINEAR,
        FILTER_ANISOTROPIC,

        NUM_FILTERS,
    };

    enum WrapMode {
        WRAP_REPEAT = 0,
        WRAP_MIRROR,
        WRAP_CLAMP,
        WRAP_BORDER,
        WRAP_MIRROR_ONCE,

        NUM_WRAP_MODES
    };

    static const uint8 MAX_MIP_LEVEL = 0xFF;

    class Desc {
    public:
        glm::vec4 _borderColor{ 1.0f };
        uint32 _maxAnisotropy = 16;

        uint8 _filter = FILTER_MIN_MAG_POINT;
        uint8 _comparisonFunc = ALWAYS;

        uint8 _wrapModeU = WRAP_REPEAT;
        uint8 _wrapModeV = WRAP_REPEAT;
        uint8 _wrapModeW = WRAP_REPEAT;
            
        uint8 _mipOffset = 0;
        uint8 _minMip = 0;
        uint8 _maxMip = MAX_MIP_LEVEL;

        Desc() {}
        Desc(const Filter filter, const WrapMode wrap = WRAP_REPEAT) : _filter(filter), _wrapModeU(wrap), _wrapModeV(wrap), _wrapModeW(wrap) {}
    };

    Sampler() {}
    Sampler(const Filter filter, const WrapMode wrap = WRAP_REPEAT) : _desc(filter, wrap) {}
    Sampler(const Desc& desc) : _desc(desc) {}
    ~Sampler() {}

    const glm::vec4& getBorderColor() const { return _desc._borderColor; }

    uint32 getMaxAnisotropy() const { return _desc._maxAnisotropy; }

    WrapMode getWrapModeU() const { return WrapMode(_desc._wrapModeU); }
    WrapMode getWrapModeV() const { return WrapMode(_desc._wrapModeV); }
    WrapMode getWrapModeW() const { return WrapMode(_desc._wrapModeW); }

    Filter getFilter() const { return Filter(_desc._filter); }
    ComparisonFunction getComparisonFunction() const { return ComparisonFunction(_desc._comparisonFunc); }
    bool doComparison() const { return getComparisonFunction() != ALWAYS; }

    uint8 getMipOffset() const { return _desc._mipOffset; }
    uint8 getMinMip() const { return _desc._minMip; }
    uint8 getMaxMip() const { return _desc._maxMip; }

    const Desc& getDesc() const { return _desc; }
protected:
    Desc _desc;
};

enum class TextureUsageType {
    RENDERBUFFER,       // Used as attachments to a framebuffer
    RESOURCE,           // Resource textures, like materials... subject to memory manipulation
    STRICT_RESOURCE,    // Resource textures not subject to manipulation, like the normal fitting texture
    EXTERNAL,
};

class Texture : public Resource {
    static std::atomic<uint32_t> _textureCPUCount;
    static std::atomic<Size> _textureCPUMemoryUsage;
    static std::atomic<Size> _allowedCPUMemoryUsage;
    static std::atomic<bool> _enableSparseTextures;
    static void updateTextureCPUMemoryUsage(Size prevObjectSize, Size newObjectSize);

public:
    static const uint32_t CUBE_FACE_COUNT { 6 };
    static uint32_t getTextureCPUCount();
    static Size getTextureCPUMemoryUsage();
    static uint32_t getTextureGPUCount();
    static uint32_t getTextureGPUSparseCount();
    static Size getTextureTransferPendingSize();
    static Size getTextureGPUMemoryUsage();
    static Size getTextureGPUVirtualMemoryUsage();
    static Size getTextureGPUFramebufferMemoryUsage();
    static Size getTextureGPUSparseMemoryUsage();
    static uint32_t getTextureGPUTransferCount();
    static Size getAllowedGPUMemoryUsage();
    static void setAllowedGPUMemoryUsage(Size size);

    static bool getEnableSparseTextures();
    static void setEnableSparseTextures(bool enabled);

    using ExternalRecycler = std::function<void(uint32, void*)>;
    using ExternalIdAndFence = std::pair<uint32, void*>;
    using ExternalUpdates = std::list<ExternalIdAndFence>;

    class Usage {
    public:
        enum FlagBit {
            COLOR = 0,   // Texture is a color map
            NORMAL,      // Texture is a normal map
            ALPHA,      // Texture has an alpha channel
            ALPHA_MASK,       // Texture alpha channel is a Mask 0/1
            NUM_FLAGS,  
        };

        typedef std::bitset<NUM_FLAGS> Flags;

        // The key is the Flags
        Flags _flags;

        Usage() : _flags(0) {}
        Usage(const Flags& flags) : _flags(flags) {}

        bool operator== (const Usage& rhs) const { return _flags == rhs._flags; }
        bool operator!= (const Usage& rhs) const { return _flags != rhs._flags; }

        class Builder {
            friend class Usage;
            Flags _flags{ 0 };
        public:
            Builder() {}

            Usage build() const { return Usage(_flags); }

            Builder& withColor() { _flags.set(COLOR); return (*this); }
            Builder& withNormal() { _flags.set(NORMAL); return (*this); }
            Builder& withAlpha() { _flags.set(ALPHA); return (*this); }
            Builder& withAlphaMask() { _flags.set(ALPHA_MASK); return (*this); }
        };
        Usage(const Builder& builder) : Usage(builder._flags) {}

        bool isColor() const { return _flags[COLOR]; }
        bool isNormal() const { return _flags[NORMAL]; }

        bool isAlpha() const { return _flags[ALPHA]; }
        bool isAlphaMask() const { return _flags[ALPHA_MASK]; }

        bool operator==(const Usage& usage) { return (_flags == usage._flags); }
        bool operator!=(const Usage& usage) { return (_flags != usage._flags); }
    };

    enum Type : uint8 {
        TEX_1D = 0,
        TEX_2D,
        TEX_3D,
        TEX_CUBE,

        NUM_TYPES,
    };

    // Definition of the cube face name and layout
    enum CubeFace {
        CUBE_FACE_RIGHT_POS_X = 0,
        CUBE_FACE_LEFT_NEG_X,
        CUBE_FACE_TOP_POS_Y,
        CUBE_FACE_BOTTOM_NEG_Y,
        CUBE_FACE_BACK_POS_Z,
        CUBE_FACE_FRONT_NEG_Z,

        NUM_CUBE_FACES, // Not a valid vace index
    };

    // Lines of pixels are padded to be a multiple of "PACKING_SIZE" which is 4 bytes
    static const uint32 PACKING_SIZE = 4;
    static uint8 evalPaddingNumBytes(Size byteSize) { return (uint8) (3 - (byteSize + 3) % PACKING_SIZE); }
    static Size evalPaddedSize(Size byteSize) { return byteSize + (Size) evalPaddingNumBytes(byteSize); }


    using PixelsPointer = storage::StoragePointer;
    class Storage {
    public:
        Storage() {}
        virtual ~Storage() {}

        virtual void reset() = 0;
        virtual PixelsPointer getMipFace(uint16 level, uint8 face = 0) const = 0;
        virtual Size getMipFaceSize(uint16 level, uint8 face = 0) const = 0;
        virtual void assignMipData(uint16 level, const storage::StoragePointer& storage) = 0;
        virtual void assignMipFaceData(uint16 level, uint8 face, const storage::StoragePointer& storage) = 0;
        virtual bool isMipAvailable(uint16 level, uint8 face = 0) const = 0;
        Texture::Type getType() const { return _type; }

        Stamp getStamp() const { return _stamp; }
        Stamp bumpStamp() { return ++_stamp; }

        void setFormat(const Element& format) { _format = format; }
        const Element& getFormat() const { return _format; }

    private:
        Stamp _stamp { 0 };
        Element _format;
        Texture::Type _type { Texture::TEX_2D }; // The type of texture is needed to know the number of faces to expect
        Texture* _texture { nullptr }; // Points to the parent texture (not owned)
        virtual void assignTexture(Texture* tex); // Texture storage is pointing to ONE corrresponding Texture.
        const Texture* getTexture() const { return _texture; }
        friend class Texture;
    };

    class MemoryStorage : public Storage {
    public:
        void reset() override;
        PixelsPointer getMipFace(uint16 level, uint8 face = 0) const override;
        Size getMipFaceSize(uint16 level, uint8 face = 0) const override;
        void assignMipData(uint16 level, const storage::StoragePointer& storage) override;
        void assignMipFaceData(uint16 level, uint8 face, const storage::StoragePointer& storage) override;
        bool isMipAvailable(uint16 level, uint8 face = 0) const override;

    protected:
        bool allocateMip(uint16 level);
        std::vector<std::vector<PixelsPointer>> _mips; // an array of mips, each mip is an array of faces
    };

    class KtxStorage : public Storage {
    public:
        KtxStorage(const std::string& filename);
        PixelsPointer getMipFace(uint16 level, uint8 face = 0) const override;
        Size getMipFaceSize(uint16 level, uint8 face = 0) const override;
        // By convention, all mip levels and faces MUST be populated when using KTX backing
        bool isMipAvailable(uint16 level, uint8 face = 0) const override { return true; }

        void assignMipData(uint16 level, const storage::StoragePointer& storage) override {
            throw std::runtime_error("Invalid call");
        }

        void assignMipFaceData(uint16 level, uint8 face, const storage::StoragePointer& storage) override {
            throw std::runtime_error("Invalid call");
        }
        void reset() override { }

    protected:
        std::string _filename;
        ktx::KTXDescriptorPointer _ktxDescriptor;
        friend class Texture;
    };

    static const uint16 MAX_NUM_MIPS = 0;
    static const uint16 SINGLE_MIP = 1;
    static Texture* create1D(const Element& texelFormat, uint16 width, uint16 numMips = SINGLE_MIP, const Sampler& sampler = Sampler());
    static Texture* create2D(const Element& texelFormat, uint16 width, uint16 height, uint16 numMips = SINGLE_MIP, const Sampler& sampler = Sampler());
    static Texture* create3D(const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numMips = SINGLE_MIP, const Sampler& sampler = Sampler());
    static Texture* createCube(const Element& texelFormat, uint16 width, uint16 numMips = 1, const Sampler& sampler = Sampler());
    static Texture* createRenderBuffer(const Element& texelFormat, uint16 width, uint16 height, uint16 numMips = SINGLE_MIP, const Sampler& sampler = Sampler());
    static Texture* createStrict(const Element& texelFormat, uint16 width, uint16 height, uint16 numMips = SINGLE_MIP, const Sampler& sampler = Sampler());
    static Texture* createExternal(const ExternalRecycler& recycler, const Sampler& sampler = Sampler());

    // After the texture has been created, it should be defined
    bool isDefined() const { return _defined; }

    Texture(TextureUsageType usageType);
    Texture(const Texture& buf); // deep copy of the sysmem texture
    Texture& operator=(const Texture& buf); // deep copy of the sysmem texture
    ~Texture();

    Stamp getStamp() const { return _stamp; }
    Stamp getDataStamp() const { return _storage->getStamp(); }

    // The theoretical size in bytes of data stored in the texture
    // For the master (level) first level of mip
    Size getSize() const override { return _size; }

    // Size and format
    Type getType() const { return _type; }
    TextureUsageType getUsageType() const { return _usageType; }

    bool isColorRenderTarget() const;
    bool isDepthStencilRenderTarget() const;

    const Element& getTexelFormat() const { return _texelFormat; }

    Vec3u getDimensions() const { return Vec3u(_width, _height, _depth); }
    uint16 getWidth() const { return _width; }
    uint16 getHeight() const { return _height; }
    uint16 getDepth() const { return _depth; }

    // The number of faces is mostly used for cube map, and maybe for stereo ? otherwise it's 1
    // For cube maps, this means the pixels of the different faces are supposed to be packed back to back in a mip
    // as if the height was NUM_FACES time bigger.
    static uint8 NUM_FACES_PER_TYPE[NUM_TYPES];
    uint8 getNumFaces() const { return NUM_FACES_PER_TYPE[getType()]; }

    // The texture is an array if the _numSlices is not 0.
    // otherwise, if _numSLices is 0, then the texture is NOT an array
    // The number of slices returned is 1 at the minimum (if not an array) or the actual _numSlices.
    bool isArray() const { return _numSlices > 0; }
    uint16 getNumSlices() const { return (isArray() ? _numSlices : 1); }

    uint16 getNumSamples() const { return _numSamples; }
    // NumSamples can only have certain values based on the hw
    static uint16 evalNumSamplesUsed(uint16 numSamplesTried);

    // max mip is in the range [ 0 if no sub mips, log2(max(width, height, depth))]
    // It is defined at creation time (immutable)
    uint16 getMaxMip() const { return _maxMipLevel; }
    uint16 getNumMips() const { return _maxMipLevel + 1; }

    // Mips size evaluation

    // The number mips that a dimension could haves
    // = 1 + log2(size)
    static uint16 evalDimMaxNumMips(uint16 size);

    // The number mips that the texture could have if all existed
    // = 1 + log2(max(width, height, depth))
    uint16 evalMaxNumMips() const;
    static uint16 evalMaxNumMips(const Vec3u& dimensions);

    // Check a num of mips requested against the maximum possible specified
    // if passing -1 then answer the max
    // simply does (askedNumMips == -1 ? maxMips : (numstd::min(askedNumMips, max))
    static uint16 safeNumMips(uint16 askedNumMips, uint16 maxMips);

    // Same but applied to this texture's num max mips from evalNumMips()
    uint16 safeNumMips(uint16 askedNumMips) const;

    // Eval the size that the mips level SHOULD have
    // not the one stored in the Texture

    Vec3u evalMipDimensions(uint16 level) const;
    uint16 evalMipWidth(uint16 level) const { return std::max(_width >> level, 1); }
    uint16 evalMipHeight(uint16 level) const { return std::max(_height >> level, 1); }
    uint16 evalMipDepth(uint16 level) const { return std::max(_depth >> level, 1); }

    // The size of a face is a multiple of the padded line = (width * texelFormat_size + alignment padding)
    Size evalMipLineSize(uint16 level) const { return evalPaddedSize(evalMipWidth(level) * getTexelFormat().getSize()); }

    // Size for each face of a mip at a particular level
    uint32 evalMipFaceNumTexels(uint16 level) const { return evalMipWidth(level) * evalMipHeight(level) * evalMipDepth(level); }
    Size evalMipFaceSize(uint16 level) const { return evalMipLineSize(level) * evalMipHeight(level) * evalMipDepth(level); }
    
    // Total size for the mip
    uint32 evalMipNumTexels(uint16 level) const { return evalMipFaceNumTexels(level) * getNumFaces(); }
    Size evalMipSize(uint16 level) const { return evalMipFaceSize(level) * getNumFaces(); }

    // Total size for all the mips of the texture
    Size evalTotalSize(uint16 startingMip = 0) const;

    // Compute the theorical size of the texture elements storage depending on the specified format
    Size evalStoredMipLineSize(uint16 level, const Element& format) const { return evalPaddedSize(evalMipWidth(level) * format.getSize()); }
    Size evalStoredMipFaceSize(uint16 level, const Element& format) const { return evalMipFaceNumTexels(level) * format.getSize(); }
    Size evalStoredMipSize(uint16 level, const Element& format) const { return evalMipNumTexels(level) * format.getSize(); }

    // For convenience assign a source name 
    const std::string& source() const { return _source; }
    void setSource(const std::string& source) { _source = source; }

    // Potentially change the minimum mip (mostly for debugging purpose)
    bool setMinMip(uint16 newMinMip);
    bool incremementMinMip(uint16 count = 1);
    uint16 getMinMip() const { return _minMip; }
    uint16 usedMipLevels() const { return (getNumMips() - _minMip); }

    // Generate the sub mips automatically for the texture
    // If the storage version is not available (from CPU memory)
    // Only works for the standard formats
    void setAutoGenerateMips(bool enable);
    bool isAutogenerateMips() const { return _autoGenerateMips; }

    // Managing Storage and mips

    // Mip storage format is constant across all mips
    void setStoredMipFormat(const Element& format);
    const Element& getStoredMipFormat() const;

    // Manually allocate the mips down until the specified maxMip
    // this is just allocating the sysmem version of it
    // in case autoGen is on, this doesn't allocate
    // Explicitely assign mip data for a certain level
    // If Bytes is NULL then simply allocate the space so mip sysmem can be accessed
    void assignStoredMip(uint16 level, Size size, const Byte* bytes);
    void assignStoredMipFace(uint16 level, uint8 face, Size size, const Byte* bytes);

    void assignStoredMip(uint16 level, storage::StoragePointer& storage);
    void assignStoredMipFace(uint16 level, uint8 face, storage::StoragePointer& storage);

    // Access the stored mips and faces
    const PixelsPointer accessStoredMipFace(uint16 level, uint8 face = 0) const { return _storage->getMipFace(level, face); }
    bool isStoredMipFaceAvailable(uint16 level, uint8 face = 0) const { return _storage->isMipAvailable(level, face); }
    Size getStoredMipFaceSize(uint16 level, uint8 face = 0) const { return _storage->getMipFaceSize(level, face); }
    Size getStoredMipSize(uint16 level) const;
    Size getStoredSize() const;

    void setStorage(std::unique_ptr<Storage>& newStorage);
    void setKtxBacking(const std::string& filename);

    // Usage is a a set of flags providing Semantic about the usage of the Texture.
    void setUsage(const Usage& usage) { _usage = usage; }
    Usage getUsage() const { return _usage; }

    // For Cube Texture, it's possible to generate the irradiance spherical harmonics and make them availalbe with the texture
    bool generateIrradiance();
    const SHPointer& getIrradiance(uint16 slice = 0) const { return _irradiance; }
    bool isIrradianceValid() const { return _isIrradianceValid; }

    // Own sampler
    void setSampler(const Sampler& sampler);
    const Sampler& getSampler() const { return _sampler; }
    Stamp getSamplerStamp() const { return _samplerStamp; }

    void setFallbackTexture(const TexturePointer& fallback) { _fallback = fallback; }
    TexturePointer getFallbackTexture() const { return _fallback.lock(); }

    void setExternalTexture(uint32 externalId, void* externalFence);
    void setExternalRecycler(const ExternalRecycler& recycler);
    ExternalRecycler getExternalRecycler() const;

    const GPUObjectPointer gpuObject {};

    ExternalUpdates getUpdates() const;

    // Textures can be serialized directly to ktx data file, here is how
    static ktx::KTXUniquePointer serialize(const Texture& texture);
    static Texture* unserialize(const std::string& ktxFile, TextureUsageType usageType = TextureUsageType::RESOURCE, Usage usage = Usage(), const Sampler::Desc& sampler = Sampler::Desc());
    static bool evalKTXFormat(const Element& mipFormat, const Element& texelFormat, ktx::Header& header);
    static bool evalTextureFormat(const ktx::Header& header, Element& mipFormat, Element& texelFormat);

protected:
    const TextureUsageType _usageType;

    // Should only be accessed internally or by the backend sync function
    mutable Mutex _externalMutex;
    mutable std::list<ExternalIdAndFence> _externalUpdates;
    ExternalRecycler _externalRecycler;


    std::weak_ptr<Texture> _fallback;
    // Not strictly necessary, but incredibly useful for debugging
    std::string _source;
    std::unique_ptr< Storage > _storage;

    Stamp _stamp { 0 };

    Sampler _sampler;
    Stamp _samplerStamp { 0 };

    Size _size { 0 };
    Element _texelFormat;

    uint16 _width { 1 };
    uint16 _height { 1 };
    uint16 _depth { 1 };

    uint16 _numSamples { 1 };

    // if _numSlices is 0, the texture is not an "Array", the getNumSlices reported is 1
    uint16 _numSlices { 0 };

    // valid _maxMipLevel is in the range [ 0 if no sub mips, log2(max(width, height, depth) ]
    // The num of mips returned is _maxMipLevel + 1
    uint16 _maxMipLevel { 0 };

    uint16 _minMip { 0 };
 
    Type _type { TEX_1D };

    Usage _usage;

    SHPointer _irradiance;
    bool _autoGenerateMips = false;
    bool _isIrradianceValid = false;
    bool _defined = false;
   
    static Texture* create(TextureUsageType usageType, Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices, uint16 numMips, const Sampler& sampler);

    Size resize(Type type, const Element& texelFormat, uint16 width, uint16 height, uint16 depth, uint16 numSamples, uint16 numSlices, uint16 numMips);
};

typedef std::shared_ptr<Texture> TexturePointer;
typedef std::vector< TexturePointer > Textures;

 // TODO: For now TextureView works with Texture as a place holder for the Texture.
 // The overall logic should be about the same except that the Texture will be a real GL Texture under the hood
class TextureView {
public:
    typedef Resource::Size Size;

    TexturePointer _texture = TexturePointer(NULL);
    uint16 _subresource = 0;
    Element _element = Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);

    TextureView() {};

    TextureView(const Element& element) :
         _element(element)
    {};

    // create the TextureView and own the Texture
    TextureView(Texture* newTexture, const Element& element) :
        _texture(newTexture),
        _subresource(0),
        _element(element)
    {};
    TextureView(const TexturePointer& texture, uint16 subresource, const Element& element) :
        _texture(texture),
        _subresource(subresource),
        _element(element)
    {};

    TextureView(const TexturePointer& texture, uint16 subresource) :
        _texture(texture),
        _subresource(subresource)
    {};

    ~TextureView() {}
    TextureView(const TextureView& view) = default;
    TextureView& operator=(const TextureView& view) = default;

    explicit operator bool() const { return bool(_texture); }
    bool operator !() const { return (!_texture); }

    bool isValid() const { return bool(_texture); }
};
typedef std::vector<TextureView> TextureViews;

// TextureSource is the bridge between a URL or a a way to produce an image and the final gpu::Texture that will be used to render it.
// It provides the mechanism to create a texture using a customizable TextureLoader
class TextureSource {
public:
    TextureSource();
    ~TextureSource();

    const QUrl& getUrl() const { return _imageUrl; }
    const gpu::TexturePointer getGPUTexture() const { return _gpuTexture; }

    void reset(const QUrl& url);

    void resetTexture(gpu::TexturePointer texture);

    bool isDefined() const;

protected:
    gpu::TexturePointer _gpuTexture;
    QUrl _imageUrl;
};
typedef std::shared_ptr< TextureSource > TextureSourcePointer;

};

Q_DECLARE_METATYPE(gpu::TexturePointer)

#endif
