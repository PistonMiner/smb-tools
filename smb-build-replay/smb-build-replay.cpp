#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <deque>
#include <chrono>

#include "json.hpp"
using json = nlohmann::json;

#include <boost/program_options.hpp>

std::vector<uint8_t> loadFile(const std::string &filename)
{
	FILE *file = fopen(filename.c_str(), "rb");
	if (!file)
	{
		return std::vector<uint8_t>();
	}
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);

	std::vector<uint8_t> buf(size);
	if (buf.size() > 0)
	{
		fseek(file, 0, SEEK_SET);
		fread(buf.data(), 1, buf.size(), file);
		fclose(file);
	}

	return buf;
}

bool saveFile(const std::string &filename, const std::vector<uint8_t> &buffer)
{
	FILE *file = fopen(filename.c_str(), "wb");
	if (!file)
	{
		return false;
	}
	fwrite(buffer.data(), 1, buffer.size(), file);
	fclose(file);
	return true;
}

std::vector<uint8_t> stringToBuffer(const std::string &buffer)
{
	std::vector<uint8_t> binaryBuffer;
	for (const auto &c : buffer)
	{
		binaryBuffer.emplace_back(c);
	}
	return binaryBuffer;
}

std::string bufferToString(const std::vector<uint8_t> &buffer)
{
	std::string outputString;
	for (const auto &c : buffer)
	{
		outputString += static_cast<char>(c);
	}
	return outputString;
}

std::vector<uint8_t> compressBufferRLE(const std::vector<uint8_t> &buffer)
{
	// First, find sequences worth compressing
	struct RepeatingRegion
	{
		RepeatingRegion(size_t offset, uint8_t value)
			: offset(offset), value(value), count(0)
		{}

		size_t offset;
		uint8_t value;
		size_t count;
	};
	std::deque<RepeatingRegion> repeatList;
	for (size_t i = 0; i < buffer.size(); ++i)
	{
		if (repeatList.empty() || repeatList.back().count >= 0x7F || buffer[i] != repeatList.back().value)
		{
			repeatList.emplace_back(i, buffer[i]);
		}
		++repeatList.back().count;
	}
	// Remove regions not worth compressing
	repeatList.erase(std::remove_if(repeatList.begin(), repeatList.end(), [](const auto &val)
	{
		return val.count <= 2;
	}), repeatList.end());
	// Write out compressed binary
	std::vector<uint8_t> compressedBuffer;
	for (size_t i = 0; i < buffer.size(); )
	{
		if (!repeatList.empty() && repeatList.front().offset == i)
		{
			const auto &region = repeatList.front();
			compressedBuffer.emplace_back(static_cast<uint8_t>(region.count | 0x80));
			compressedBuffer.emplace_back(region.value);
			i += region.count;
			repeatList.pop_front();
		}
		else
		{
			// Write to end of buffer or to next compressed region
			size_t length = repeatList.empty() ? buffer.size() - i : repeatList.front().offset - i;
			// Can only write up to 0x7F bytes contiguously
			for (size_t j = 0; j < length; j += 0x7F)
			{
				size_t tagLength = std::min(static_cast<size_t>(length - j), static_cast<size_t>(0x7Fu));
				compressedBuffer.emplace_back(static_cast<uint8_t>(tagLength));
				auto sourceIt = buffer.begin() + i + j;
				compressedBuffer.insert(compressedBuffer.end(), sourceIt, sourceIt + tagLength);
			}
			i += length;
		}
	}
	return compressedBuffer;
}

std::vector<uint8_t> decompressBufferRLE(const std::vector<uint8_t> &buffer, size_t decompressedSize)
{
	std::vector<uint8_t> decompressedBuffer;
	for (size_t i = 0; i < buffer.size() && decompressedBuffer.size() < decompressedSize; )
	{
		if (buffer[i] & 0x80)
		{
			decompressedBuffer.insert(decompressedBuffer.end(), buffer[i] & ~0x80, buffer[i + 1]);
			i += 2;
		}
		else
		{
			// Make room
			auto sourceIt = buffer.begin() + i + 1;
			decompressedBuffer.insert(decompressedBuffer.end(), sourceIt, sourceIt + buffer[i]);
			i += buffer[i] + 1;
		}
	}
	return decompressedBuffer;
}

uint16_t getCRCForBuffer(const std::vector<uint8_t> &buffer)
{
	const uint16_t polynomial = 0x1021;
	uint16_t checksum = 0xFFFF;
	for (const auto &val : buffer)
	{
		checksum ^= (val << 8);

		for (size_t i = 0; i < 8; ++i)
		{
			if (checksum & 0x8000)
			{
				checksum <<= 1;
				checksum ^= polynomial;
			}
			else
			{
				checksum <<= 1;
			}
		}
	}
	checksum = ~checksum;
	return checksum;
}

template<typename T>
void serializeBinary(std::vector<uint8_t> &buffer, const T &value)
{
	for (size_t i = sizeof(T); i > 0; --i)
	{
		buffer.emplace_back(static_cast<uint8_t>((value >> (i - 1) * 8) & 0xFF));
	}
}

template<typename T>
void deserializeBinary(std::vector<uint8_t> &buffer, T &value)
{
	value = 0;
	for (size_t i = sizeof(T); i > 0; --i)
	{
		value |= static_cast<T>(buffer.front()) << ((i - 1) * 8);
		buffer.erase(buffer.begin());
	}
}

template<typename T>
void serializeJSON(nlohmann::json &buffer, const std::string &name, const T &value)
{
	buffer[name] = value;
}

template<typename T>
void deserializeJSON(const nlohmann::json &buffer, const std::string &name, T &value)
{
	value = buffer[name];
}

template<typename T>
void serializeBinary(std::vector<uint8_t> &buffer, const std::vector<T> &vector)
{
	for (const auto &element : vector)
	{
		serializeBinary(buffer, element);
	}
}

template<typename T>
void deserializeBinary(std::vector<uint8_t> &buffer, std::vector<T> &vector)
{
	for (auto &element : vector)
	{
		deserializeBinary(buffer, element);
	}
}

template<typename T>
void serializeJSON(nlohmann::json &buffer, const std::string &name, const std::vector<T> &vector)
{
	for (const auto &element : vector)
	{
		buffer[name].emplace_back(element);
	}
}

template<typename T>
void deserializeJSON(const nlohmann::json &buffer, const std::string &name, std::vector<T> &vector)
{
	for (size_t i = 0; i < vector.size() && i < buffer[name].size(); ++i)
	{
		vector[i] = static_cast<T>(buffer[name][i]);
	}
}

template<>
void serializeBinary(std::vector<uint8_t> &buffer, const float &value)
{
	const uint32_t &rawValue = reinterpret_cast<const uint32_t &>(value);
	for (size_t i = 4; i > 0; --i)
	{
		buffer.emplace_back((rawValue >> (i - 1) * 8) & 0xFF);
	}
}

template<>
void deserializeBinary(std::vector<uint8_t> &buffer, float &value)
{
	uint32_t rawValue;
	rawValue = 0;
	for (size_t i = 4; i > 0; --i)
	{
		rawValue |= static_cast<uint32_t>(buffer.front()) << (i - 1) * 8;
		buffer.erase(buffer.begin());
	}
	value = *reinterpret_cast<float *>(&rawValue);
}

struct ReplayFileHeader
{
	uint16_t flags;
	uint8_t levelID;
	uint8_t levelDifficulty;
	uint8_t levelFloor;
	uint8_t  monkeyType;
	uint16_t unk_06;
	uint32_t unk_08;
	uint32_t unk_0c;
	uint32_t scorePoints;
	uint32_t unk_14;
	uint16_t levelMaxTime;
	uint16_t replayTotalTime;
	uint16_t scoreTimeRemaining;
	uint16_t unk_1E;
	uint32_t timeWithScore;
	float	 unk_24;
	float	 unk_28;
	float	 unk_2c;
	uint32_t unk_30;
	uint32_t unk_34; // same as replayTotalTime in all examples
	float	 startPositionX;
	float	 startPositionY;
	float	 startPositionZ;
};

template<>
void serializeBinary<ReplayFileHeader>(std::vector<uint8_t> &buffer, const ReplayFileHeader &value)
{
	serializeBinary(buffer, value.flags);
	serializeBinary(buffer, value.levelID);
	serializeBinary(buffer, value.levelDifficulty);
	serializeBinary(buffer, value.levelFloor);
	serializeBinary(buffer, value.monkeyType);
	serializeBinary(buffer, value.unk_06);
	serializeBinary(buffer, value.unk_08);
	serializeBinary(buffer, value.unk_0c);
	serializeBinary(buffer, value.scorePoints);
	serializeBinary(buffer, value.unk_14);
	serializeBinary(buffer, value.levelMaxTime);
	serializeBinary(buffer, value.replayTotalTime);
	serializeBinary(buffer, value.scoreTimeRemaining);
	serializeBinary(buffer, value.unk_1E);
	serializeBinary(buffer, value.timeWithScore);
	serializeBinary(buffer, value.unk_24);
	serializeBinary(buffer, value.unk_28);
	serializeBinary(buffer, value.unk_2c);
	serializeBinary(buffer, value.unk_30);
	serializeBinary(buffer, value.unk_34);
	serializeBinary(buffer, value.startPositionX);
	serializeBinary(buffer, value.startPositionY);
	serializeBinary(buffer, value.startPositionZ);
}

template<>
void deserializeBinary<ReplayFileHeader>(std::vector<uint8_t> &buffer, ReplayFileHeader &value)
{
	deserializeBinary(buffer, value.flags);
	deserializeBinary(buffer, value.levelID);
	deserializeBinary(buffer, value.levelDifficulty);
	deserializeBinary(buffer, value.levelFloor);
	deserializeBinary(buffer, value.monkeyType);
	deserializeBinary(buffer, value.unk_06);
	deserializeBinary(buffer, value.unk_08);
	deserializeBinary(buffer, value.unk_0c);
	deserializeBinary(buffer, value.scorePoints);
	deserializeBinary(buffer, value.unk_14);
	deserializeBinary(buffer, value.levelMaxTime);
	deserializeBinary(buffer, value.replayTotalTime);
	deserializeBinary(buffer, value.scoreTimeRemaining);
	deserializeBinary(buffer, value.unk_1E);
	deserializeBinary(buffer, value.timeWithScore);
	deserializeBinary(buffer, value.unk_24);
	deserializeBinary(buffer, value.unk_28);
	deserializeBinary(buffer, value.unk_2c);
	deserializeBinary(buffer, value.unk_30);
	deserializeBinary(buffer, value.unk_34);
	deserializeBinary(buffer, value.startPositionX);
	deserializeBinary(buffer, value.startPositionY);
	deserializeBinary(buffer, value.startPositionZ);
}

template<>
void serializeJSON<ReplayFileHeader>(nlohmann::json &buffer, const std::string &name, const ReplayFileHeader &value)
{
	serializeJSON(buffer[name], "flags", value.flags);
	serializeJSON(buffer[name], "levelID", value.levelID);
	serializeJSON(buffer[name], "levelDifficulty", value.levelDifficulty);
	serializeJSON(buffer[name], "levelFloor", value.levelFloor);
	serializeJSON(buffer[name], "monkeyType", value.monkeyType);
	serializeJSON(buffer[name], "unk_06", value.unk_06);
	serializeJSON(buffer[name], "unk_08", value.unk_08);
	serializeJSON(buffer[name], "unk_0c", value.unk_0c);
	serializeJSON(buffer[name], "scorePoints", value.scorePoints);
	serializeJSON(buffer[name], "unk_14", value.unk_14);
	serializeJSON(buffer[name], "levelMaxTime", value.levelMaxTime);
	serializeJSON(buffer[name], "replayTotalTime", value.replayTotalTime);
	serializeJSON(buffer[name], "scoreTimeRemaining", value.scoreTimeRemaining);
	serializeJSON(buffer[name], "unk_1E", value.unk_1E);
	serializeJSON(buffer[name], "timeWithScore", value.timeWithScore);
	serializeJSON(buffer[name], "unk_24", value.unk_24);
	serializeJSON(buffer[name], "unk_28", value.unk_28);
	serializeJSON(buffer[name], "unk_2c", value.unk_2c);
	serializeJSON(buffer[name], "unk_30", value.unk_30);
	serializeJSON(buffer[name], "unk_34", value.unk_34);
	serializeJSON(buffer[name], "startPositionX", value.startPositionX);
	serializeJSON(buffer[name], "startPositionY", value.startPositionY);
	serializeJSON(buffer[name], "startPositionZ", value.startPositionZ);
}

template<>
void deserializeJSON<ReplayFileHeader>(const nlohmann::json &buffer, const std::string &name, ReplayFileHeader &value)
{
	deserializeJSON(buffer[name], "flags", value.flags);
	deserializeJSON(buffer[name], "levelID", value.levelID);
	deserializeJSON(buffer[name], "levelDifficulty", value.levelDifficulty);
	deserializeJSON(buffer[name], "levelFloor", value.levelFloor);
	deserializeJSON(buffer[name], "monkeyType", value.monkeyType);
	deserializeJSON(buffer[name], "unk_06", value.unk_06);
	deserializeJSON(buffer[name], "unk_08", value.unk_08);
	deserializeJSON(buffer[name], "unk_0c", value.unk_0c);
	deserializeJSON(buffer[name], "scorePoints", value.scorePoints);
	deserializeJSON(buffer[name], "unk_14", value.unk_14);
	deserializeJSON(buffer[name], "levelMaxTime", value.levelMaxTime);
	deserializeJSON(buffer[name], "replayTotalTime", value.replayTotalTime);
	deserializeJSON(buffer[name], "scoreTimeRemaining", value.scoreTimeRemaining);
	deserializeJSON(buffer[name], "unk_1E", value.unk_1E);
	deserializeJSON(buffer[name], "timeWithScore", value.timeWithScore);
	deserializeJSON(buffer[name], "unk_24", value.unk_24);
	deserializeJSON(buffer[name], "unk_28", value.unk_28);
	deserializeJSON(buffer[name], "unk_2c", value.unk_2c);
	deserializeJSON(buffer[name], "unk_30", value.unk_30);
	deserializeJSON(buffer[name], "unk_34", value.unk_34);
	deserializeJSON(buffer[name], "startPositionX", value.startPositionX);
	deserializeJSON(buffer[name], "startPositionY", value.startPositionY);
	deserializeJSON(buffer[name], "startPositionZ", value.startPositionZ);
}

struct ReplayFile
{
	ReplayFileHeader header;
	std::vector<std::vector<float>> playerPositionDelta;
	std::vector<std::vector<float>> playerTilt;
	std::vector<std::vector<float>> data567;
	std::vector<float> data8;
	std::vector<uint32_t> flags;
	std::vector<std::vector<float>> stageTilt;

	static const size_t cChunkSize = 0xF00;
	static const float cPlayerPositionDeltaScale;
	static const float cPlayerTiltScale;
	static const float cData567Scale;
	static const float cData8Scale;
	static const float cStageTiltScale;
};

const float ReplayFile::cPlayerPositionDeltaScale = 1.f / 16383.f;
const float ReplayFile::cPlayerTiltScale = 180.f / 32767.f;
const float ReplayFile::cData567Scale = 256.f;
const float ReplayFile::cData8Scale = 1.f / 127.f;
const float ReplayFile::cStageTiltScale = 90.f / 32767.f;

template<typename T>
void serializeCompoundBlock(std::vector<uint8_t> &buffer, const std::vector<T> &data)
{
	std::vector<std::vector<uint8_t>> segmentData(sizeof(T));

	for (const auto &value : data)
	{
		for (size_t i = 0; i < segmentData.size(); ++i)
		{
			segmentData[i].emplace_back((value >> (i * 8)) & 0xFF);
		}
	}

	for (auto &segment : segmentData)
	{
		serializeBinary(buffer, segment);
	}
}

template<typename T>
void deserializeCompoundBlock(std::vector<uint8_t> &buffer, std::vector<T> &data)
{
	std::vector<std::vector<uint8_t>> segmentData(sizeof(T));
	for (auto &segment : segmentData)
	{
		segment.resize(ReplayFile::cChunkSize);
		deserializeBinary(buffer, segment);
	}
	for (size_t i = 0; i < data.size(); ++i)
	{
		T val = 0;
		for (size_t j = 0; j < segmentData.size(); ++j)
		{
			val |= (static_cast<T>(segmentData[j][i]) & 0xFF) << (j * 8);
		}
		data[i] = val;
	}
}

template<typename Src, typename Dst>
void serializeScaledCompoundBlock(std::vector<uint8_t> &buffer, const std::vector<Dst> &vector, Dst scale)
{
	std::vector<Src> rawData(vector.size());
	std::transform(vector.begin(), vector.end(), rawData.begin(), [=](const auto &val)
	{
		return static_cast<Src>(val / scale);
	});
	serializeCompoundBlock(buffer, rawData);
}

template<typename Src, typename Dst>
void deserializeScaledCompoundBlock(std::vector<uint8_t> &buffer, std::vector<Dst> &vector, Dst scale)
{
	std::vector<Src> rawData(vector.size());
	deserializeCompoundBlock(buffer, rawData);
	std::transform(rawData.begin(), rawData.end(), vector.begin(), [=](const auto &val)
	{
		return static_cast<Dst>(val) * scale;
	});
}

template<typename Src, typename Dst>
void serializeScaledCompoundBlockVector(std::vector<uint8_t> &buffer, const std::vector<std::vector<Dst>> &vector, Dst scale, size_t dimensions)
{
	std::vector<std::vector<Dst>> seperatedComponents(dimensions);
	for (size_t i = 0; i < vector.size(); ++i)
	{
		for (size_t j = 0; j < seperatedComponents.size(); ++j)
		{
			seperatedComponents[j].emplace_back(vector[i][j]);
		}
	}
	for (const auto &component : seperatedComponents)
	{
		serializeScaledCompoundBlock<Src, Dst>(buffer, component, scale);
	}
}

template<typename Src, typename Dst>
void deserializeScaledCompoundBlockVector(std::vector<uint8_t> &buffer, std::vector<std::vector<Dst>> &vector, Dst scale, size_t dimensions)
{
	std::vector<std::vector<Dst>> seperatedComponents(dimensions);
	for (auto &component : seperatedComponents)
	{
		component.resize(vector.size());
		deserializeScaledCompoundBlock<Src, Dst>(buffer, component, scale);
	}
	for (size_t i = 0; i < vector.size(); ++i)
	{
		for (size_t j = 0; j < seperatedComponents.size(); ++j)
		{
			vector[i].emplace_back(seperatedComponents[j][i]);
		}
	}
}

template<>
void serializeBinary<ReplayFile>(std::vector<uint8_t> &buffer, const ReplayFile &value)
{
	serializeBinary(buffer, value.header);

	serializeScaledCompoundBlockVector<int16_t, float>(buffer,
													   value.playerPositionDelta,
													   ReplayFile::cPlayerPositionDeltaScale,
													   3);
	serializeScaledCompoundBlockVector<int16_t, float>(buffer,
													   value.playerTilt,
													   ReplayFile::cPlayerTiltScale,
													   3);
	serializeScaledCompoundBlockVector<int8_t, float>(buffer,
													  value.data567,
													  ReplayFile::cData567Scale,
													  3);
	serializeScaledCompoundBlock<int8_t, float>(buffer,
												value.data8,
												ReplayFile::cData8Scale);
	serializeCompoundBlock(buffer, value.flags);
	serializeScaledCompoundBlockVector<int16_t, float>(buffer,
													   value.stageTilt,
													   ReplayFile::cStageTiltScale,
													   2);
}

template<>
void deserializeBinary<ReplayFile>(std::vector<uint8_t> &buffer, ReplayFile &value)
{
	deserializeBinary(buffer, value.header);
	
	value.playerPositionDelta.resize(ReplayFile::cChunkSize);
	deserializeScaledCompoundBlockVector<int16_t, float>(buffer,
														 value.playerPositionDelta,
														 ReplayFile::cPlayerPositionDeltaScale,
														 3);
	value.playerTilt.resize(ReplayFile::cChunkSize);
	deserializeScaledCompoundBlockVector<int16_t, float>(buffer,
														 value.playerTilt,
														 ReplayFile::cPlayerTiltScale,
														 3);
	value.data567.resize(ReplayFile::cChunkSize);
	deserializeScaledCompoundBlockVector<int8_t, float>(buffer,
														value.data567,
														ReplayFile::cData567Scale,
														3);
	value.data8.resize(ReplayFile::cChunkSize);
	deserializeScaledCompoundBlock<int8_t, float>(buffer,
												  value.data8,
												  ReplayFile::cData8Scale);
	value.flags.resize(ReplayFile::cChunkSize);
	deserializeCompoundBlock(buffer, value.flags);
	value.stageTilt.resize(ReplayFile::cChunkSize);
	deserializeScaledCompoundBlockVector<int16_t, float>(buffer,
														 value.stageTilt,
														 ReplayFile::cStageTiltScale,
														 2);
}

template<>
void serializeJSON<ReplayFile>(nlohmann::json &buffer, const std::string &name, const ReplayFile &value)
{
	serializeJSON(buffer[name], "header", value.header);
	serializeJSON(buffer[name], "playerPositionDelta", value.playerPositionDelta);
	serializeJSON(buffer[name], "playerTilt", value.playerTilt);
	serializeJSON(buffer[name], "data567", value.data567);
	serializeJSON(buffer[name], "data8", value.data8);
	serializeJSON(buffer[name], "stageTilt", value.stageTilt);
	serializeJSON(buffer[name], "flags", value.flags);
}

template<>
void deserializeJSON<ReplayFile>(const nlohmann::json &buffer, const std::string &name, ReplayFile &value)
{
	deserializeJSON(buffer[name], "header", value.header);
	for (const std::vector<float> &it : buffer[name]["playerPositionDelta"])
		value.playerPositionDelta.emplace_back(it);
	for (const std::vector<float> &it : buffer[name]["playerTilt"])
		value.playerTilt.emplace_back(it);
	for (const std::vector<float> &it : buffer[name]["data567"])
		value.data567.emplace_back(it);
	value.data8.resize(ReplayFile::cChunkSize);
	deserializeJSON(buffer[name], "data8", value.data8);
	for (const std::vector<float> &it : buffer[name]["stageTilt"])
		value.stageTilt.emplace_back(it);
	value.flags.resize(ReplayFile::cChunkSize);
	deserializeJSON(buffer[name], "flags", value.flags);
}

struct GCIFile
{
	uint32_t gameCode = 0x474D4245; // "GMBE" #todo-smb-build-replay: Support multiple regions
	uint16_t makerCode = 0x3850; // "8P"
	//uint8_t unused_06 = 0xFF;
	uint8_t bannerFlags = 0x2; // RGB5A3 format
	std::string filename = "";
	uint32_t modifiedTime = 0x0;
	uint32_t imageOffset = 0x10; // Constant for SMB replays
	uint16_t iconFormat = 0x2; // RGB5A3 format
	uint16_t animationSpeed = 0x3; // 12 frames
	uint8_t permissions = 0x4; // no move
	uint8_t copyCounter = 0x0;
	uint16_t firstBlockNumber = 0x0;
	uint16_t blockCount = 0x0;
	//uint16_t unused_3A = 0xFFFF;
	uint32_t commentsAddress = 0x2010;

	const static size_t cReplayDataOffset = 0x2090;
	const static size_t cBlockSize = 0x2000;
	const static size_t cCommentFieldSize = 0x20;
	const static std::string cGameName;
};

const std::string GCIFile::cGameName = "Super Monkey Ball";

template<>
void serializeBinary<GCIFile>(std::vector<uint8_t> &buffer, const GCIFile &value)
{
	serializeBinary(buffer, value.gameCode);
	serializeBinary(buffer, value.makerCode);
	serializeBinary(buffer, static_cast<uint8_t>(0xFF));
	serializeBinary(buffer, value.bannerFlags);
	auto filenameBuffer = stringToBuffer(value.filename);
	filenameBuffer.resize(0x20, 0);
	serializeBinary(buffer, filenameBuffer);
	serializeBinary(buffer, value.modifiedTime);
	serializeBinary(buffer, value.imageOffset);
	serializeBinary(buffer, value.iconFormat);
	serializeBinary(buffer, value.animationSpeed);
	serializeBinary(buffer, value.permissions);
	serializeBinary(buffer, value.copyCounter);
	serializeBinary(buffer, value.firstBlockNumber);
	serializeBinary(buffer, value.blockCount);
	serializeBinary(buffer, static_cast<uint16_t>(0xFFFF));
	serializeBinary(buffer, value.commentsAddress);
}

int main(int argc, char **argv)
{
	enum class FileFormat
	{
		Unknown,
		Binary,
		JSON,
		GCI,
	};

	const std::map<std::string, FileFormat> fileFormatMap = {
		{ "binary", FileFormat::Binary },
		{ "json", FileFormat::JSON },
		{ "gci", FileFormat::GCI },
	};

	auto getFileFormatByName = [=](const std::string &name)
	{
		auto it = fileFormatMap.find(name);
		if (it == fileFormatMap.end())
		{
			return FileFormat::Unknown;
		}
		else
		{
			return it->second;
		}
	};

	namespace po = boost::program_options;
	po::options_description optionDescription("Valid options");
	optionDescription.add_options()
		("help"									, "print usage")
		("in-format,i",	po::value<std::string>(), "input file format (binary, gci, json)")
		("out-format,o",po::value<std::string>(), "output file format (binary, gci, json)")
		("in-file",		po::value<std::string>(), "input filename")
		("out-file",	po::value<std::string>(), "output filename");
	po::positional_options_description positionalOptionDescription;
	//positionalOptionDescription.add("in-format", 1);
	//positionalOptionDescription.add("out-format", 1);
	positionalOptionDescription.add("in-file", 1);
	positionalOptionDescription.add("out-file", 1);
	std::vector<std::string> unrecognizedOptions;

	bool parsingError = false;
	po::variables_map varMap;
	try
	{
		po::parsed_options parsedOptions = po::command_line_parser(argc, argv)
			.options(optionDescription)
			.positional(positionalOptionDescription)
			.allow_unregistered().run();
		unrecognizedOptions = po::collect_unrecognized(parsedOptions.options, po::exclude_positional);
		po::store(parsedOptions, varMap);
		po::notify(varMap);
	}
	catch (const boost::exception &)
	{
		parsingError = true;
	}

	if (parsingError || unrecognizedOptions.size()
		|| varMap.count("help")
		|| varMap.count("in-format") != 1
		|| varMap.count("out-format") != 1
		|| varMap.count("in-file") != 1
		|| varMap.count("out-file") != 1)
	{
		optionDescription.print(std::cout);
		return 1;
	}
	
	ReplayFile replay;
	auto inputData = loadFile(varMap.at("in-file").as<std::string>());
	if (!inputData.size())
	{
		std::cout << "Failed to read input file!" << std::endl;
		return -1;
	}

	FileFormat inputFormat = getFileFormatByName(varMap.at("in-format").as<std::string>());
	if (inputFormat == FileFormat::Binary)
	{
		deserializeBinary(inputData, replay);
	}
	else if (inputFormat == FileFormat::JSON)
	{
		nlohmann::json inputJSON = json::parse(bufferToString(inputData));
		deserializeJSON(inputJSON, "root", replay);
	}
	else if (inputFormat == FileFormat::GCI)
	{
		inputData.erase(inputData.begin(), inputData.begin() + GCIFile::cReplayDataOffset);
		uint64_t decompressedSize;
		deserializeBinary(inputData, decompressedSize);
		auto decompressedData = decompressBufferRLE(inputData, static_cast<size_t>(decompressedSize));
		deserializeBinary(decompressedData, replay);
	}
	else
	{
		std::cout << "Unknown input format!" << std::endl;
		return -1;
	}

	FileFormat outputFormat = getFileFormatByName(varMap.at("out-format").as<std::string>());
	std::vector<uint8_t> outputData;
	if (outputFormat == FileFormat::Binary)
	{
		serializeBinary(outputData, replay);
	}
	else if (outputFormat == FileFormat::JSON)
	{
		nlohmann::json outputJSON;
		serializeJSON(outputJSON, "root", replay);
		outputData = stringToBuffer(outputJSON.dump());
	}
	else if (outputFormat == FileFormat::GCI)
	{
		std::vector<uint8_t> uncompressedBuffer;
		serializeBinary(uncompressedBuffer, replay);
		auto compressedBuffer = compressBufferRLE(uncompressedBuffer);
		
		size_t finalSize = compressedBuffer.size() + GCIFile::cReplayDataOffset + sizeof(uint64_t);
		size_t blockCount = ((finalSize + GCIFile::cBlockSize - 1) & ~(GCIFile::cBlockSize - 1)) / 0x2000;

		std::vector<uint8_t> dataBuffer;
		serializeBinary(dataBuffer, replay.header.flags);
		serializeBinary(dataBuffer, replay.header.levelID);
		serializeBinary(dataBuffer, replay.header.levelDifficulty);
		serializeBinary(dataBuffer, replay.header.levelFloor);
		serializeBinary(dataBuffer, static_cast<uint8_t>(0));
		serializeBinary(dataBuffer, replay.header.scorePoints);
		serializeBinary(dataBuffer, static_cast<uint32_t>(0)); // timestamp
		dataBuffer.insert(dataBuffer.end(), ((96 * 32) + (32 * 32)) * 2, 0xCC); // some color
		
		std::vector<uint8_t> gameNameComment = stringToBuffer(GCIFile::cGameName);
		gameNameComment.resize(GCIFile::cCommentFieldSize, 0);
		dataBuffer.insert(dataBuffer.end(), gameNameComment.begin(), gameNameComment.end());

		std::string replayName;
		switch (replay.header.levelDifficulty)
		{
		case 0:
			replayName.append("Beg");
			break;
		case 1:
			replayName.append("Adv");
			break;
		case 2:
			replayName.append("Exp");
			break;
		default:
			replayName.append("Unk");
			break;
		}
		replayName.append(".FL").append(std::to_string(replay.header.levelFloor));
		replayName.append(" - smb-build-replay");

		std::vector<uint8_t> fileNameComment = stringToBuffer(replayName);
		fileNameComment.resize(GCIFile::cCommentFieldSize, 0);
		dataBuffer.insert(dataBuffer.end(), fileNameComment.begin(), fileNameComment.end());
		serializeBinary(dataBuffer, static_cast<uint64_t>(uncompressedBuffer.size()));
		dataBuffer.insert(dataBuffer.end(), compressedBuffer.begin(), compressedBuffer.end());
		dataBuffer.resize(blockCount * GCIFile::cBlockSize - sizeof(uint16_t), 0);

		GCIFile gci;
		gci.blockCount = static_cast<uint16_t>(blockCount);

		// We fill out this one so that files don't collide.
		// Not accurate since the GC epoch is 2000 and not 1970, but unique.
		uint64_t timestamp;
		{
			using namespace std::chrono;
			timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() * 40500;
		}
		char filename[21];
		snprintf(filename, sizeof(filename), "smkb%016llx", timestamp);

		gci.filename = std::string(filename);
		serializeBinary(outputData, gci);
		serializeBinary(outputData, getCRCForBuffer(dataBuffer));
		outputData.insert(outputData.end(), dataBuffer.begin(), dataBuffer.end());
	}
	else
	{
		std::cout << "Unknown output format!" << std::endl;
		return -1;
	}
	if (!saveFile(varMap.at("out-file").as<std::string>(), outputData))
	{
		std::cout << "Failed to write output file!" << std::endl;
	}

	return 0;
}
