//
//  VoxelPacketData.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 11/19/2013
//
//  TO DO:
//
//    *  add stats tracking for number of unique colors and consecutive identical colors.
//          (as research for color dictionaries and RLE)
//
//    *  further testing of compression to determine optimal configuration for performance and compression
//
//    *  improve semantics for "reshuffle" - current approach will work for now and with compression
//       but wouldn't work with RLE because the colors in the levels would get reordered and RLE would need
//       to be recalculated
//

#ifndef __hifi__VoxelPacketData__
#define __hifi__VoxelPacketData__

#include <SharedUtil.h>
#include "VoxelConstants.h"
#include "VoxelNode.h"

typedef unsigned char VOXEL_PACKET_FLAGS;
typedef uint16_t VOXEL_PACKET_SEQUENCE;
typedef uint64_t VOXEL_PACKET_SENT_TIME;
typedef uint16_t VOXEL_PACKET_INTERNAL_SECTION_SIZE;
const int MAX_VOXEL_PACKET_SIZE = MAX_PACKET_SIZE;
const int VOXEL_PACKET_HEADER_SIZE = (sizeof(PACKET_TYPE) + sizeof(PACKET_VERSION) + sizeof(VOXEL_PACKET_FLAGS) 
                + sizeof(VOXEL_PACKET_SEQUENCE) + sizeof(VOXEL_PACKET_SENT_TIME));

const int MAX_VOXEL_PACKET_DATA_SIZE = MAX_PACKET_SIZE - VOXEL_PACKET_HEADER_SIZE;
            
const int MAX_VOXEL_UNCOMRESSED_PACKET_SIZE = MAX_VOXEL_PACKET_DATA_SIZE;

const int PACKET_IS_COLOR_BIT = 0;
const int PACKET_IS_COMPRESSED_BIT = 1;

class LevelDetails {
    LevelDetails(int startIndex, int bytesOfOctalCodes, int bytesOfBitmasks, int bytesOfColor) :
        _startIndex(startIndex),
        _bytesOfOctalCodes(bytesOfOctalCodes),
        _bytesOfBitmasks(bytesOfBitmasks),
        _bytesOfColor(bytesOfColor) {
    }
    
    friend class VoxelPacketData;

private:
    int _startIndex;
    int _bytesOfOctalCodes;
    int _bytesOfBitmasks;
    int _bytesOfColor;
};

class VoxelPacketData {
public:
    VoxelPacketData(bool enableCompression = false, int maxFinalizedSize = MAX_VOXEL_PACKET_DATA_SIZE);
    ~VoxelPacketData();

    /// change compression and target size settings
    void changeSettings(bool enableCompression = false, int targetSize = MAX_VOXEL_PACKET_DATA_SIZE);

    /// reset completely, all data is discarded
    void reset();
    
    /// call to begin encoding a subtree starting at this point, this will append the octcode to the uncompressed stream
    /// at this point. May fail if new datastream is too long. In failure case the stream remains in it's previous state.
    bool startSubTree(const unsigned char* octcode = NULL);
    
    // call to indicate that the current subtree is complete and changes should be committed.
    void endSubTree();

    // call rollback the current subtree and restore the stream to the state prior to starting the subtree encoding
    void discardSubTree();

    /// starts a level marker. returns an opaque key which can be used to discard the level
    LevelDetails startLevel();

    /// discards all content back to a previous marker key
    void discardLevel(LevelDetails key);
    
    /// ends a level, and performs any expensive finalization. may fail if finalization creates a stream which is too large
    /// if the finalization would fail, the packet will automatically discard the previous level.
    bool endLevel(LevelDetails key);

    /// appends a bitmask to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendBitMask(unsigned char bitmask);

    /// updates the value of a bitmask from a previously appended portion of the uncompressed stream, might fail if the new 
    /// bitmask would cause packet to be less compressed, or if offset was out of range.
    bool updatePriorBitMask(int offset, unsigned char bitmask); 

    /// updates the uncompressed content of the stream starting at byte offset with replacementBytes for length.
    /// Might fail if the new bytes would cause packet to be less compressed, or if offset and length was out of range.
    bool updatePriorBytes(int offset, const unsigned char* replacementBytes, int length);

    /// appends a color to the end of the stream, may fail if new data stream is too long to fit in packet
    bool appendColor(const nodeColor& color);

    /// returns a byte offset from beginning of the uncompressed stream based on offset from end. 
    /// Positive offsetFromEnd returns that many bytes before the end of uncompressed stream
    int getUncompressedByteOffset(int offsetFromEnd = 0) const { return _bytesInUse - offsetFromEnd; }

    /// get access to the finalized data (it may be compressed or rewritten into optimal form)
    const unsigned char* getFinalizedData();
    /// get size of the finalized data (it may be compressed or rewritten into optimal form)
    int getFinalizedSize();

    /// get pointer to the start of uncompressed stream buffer
    const unsigned char* getUncompressedData() { return &_uncompressed[0]; }
    /// the size of the packet in uncompressed form
    int getUncompressedSize() { return _bytesInUse; }

    /// has some content been written to the packet
    bool hasContent() const { return (_bytesInUse > 0); }

    /// load finalized content to allow access to decoded content for parsing
    void loadFinalizedContent(const unsigned char* data, int length);
    
    bool isCompressed() const { return _enableCompression; }

    void debugContent();

    static uint64_t _checkCompressTime;
    static uint64_t _checkCompressCalls;

    static uint64_t _totalBytesOfOctalCodes;
    static uint64_t _totalBytesOfBitMasks;
    static uint64_t _totalBytesOfColor;
     
private:
    /// appends raw bytes, might fail if byte would cause packet to be too large
    bool append(const unsigned char* data, int length);
    
    /// append a single byte, might fail if byte would cause packet to be too large
    bool append(unsigned char byte);

    int _targetSize;
    bool _enableCompression;
    
    unsigned char _uncompressed[MAX_VOXEL_UNCOMRESSED_PACKET_SIZE];
    int _bytesInUse;
    int _bytesAvailable;
    int _subTreeAt;

    bool checkCompress();
    
    unsigned char _compressed[MAX_VOXEL_UNCOMRESSED_PACKET_SIZE];
    int _compressedBytes;
    int _bytesInUseLastCheck;
    bool _dirty;

    // statistics...
    int _bytesOfOctalCodes;
    int _bytesOfBitMasks;
    int _bytesOfColor;
    int _bytesOfOctalCodesCurrentSubTree;

    static bool _debug;
    
};

#endif /* defined(__hifi__VoxelPacketData__) */