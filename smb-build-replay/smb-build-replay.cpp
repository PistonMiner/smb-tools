// Copyright (c) 2017 Linus S. (aka PistonMiner)

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <vector>
#include <array>

#include "json.hpp"
using json = nlohmann::json;

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

/*enum class DataBlockType
{
	DeltaX0,
	DeltaX1,
	DeltaY0,
	DeltaY1,
	DeltaZ0,
	DeltaZ1,
	PlayerTiltX0,
	PlayerTiltX1,
	PlayerTiltY0,
	PlayerTiltY1,
	PlayerTiltZ0,
	PlayerTiltZ1,
	Data5,
	Data6,
	Data7,
	Data8,
	Flags0,
	Flags1,
	Flags2,
	Flags3,
	StageTiltX0,
	StageTiltX1,
	StageTiltZ0,
	StageTiltZ1,
	DataBlockType_Count,
};

enum DataType
{
	PositionDelta,
	PlayerTilt,
	Data567,
	Data8,
	Flags,
	StageTilt,
	DataType_Count,
};*/

template<typename T>
void serializeBinary(std::vector<uint8_t> &buffer, const T &value)
{
	for (size_t i = sizeof(T); i > 0; --i)
	{
		buffer.emplace_back((value >> (i - 1) * 8) & 0xFF);
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
	uint8_t  unk_05;
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
	serializeBinary(buffer, value.unk_05);
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
	deserializeBinary(buffer, value.unk_05);
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
	serializeJSON(buffer[name], "unk_05", value.unk_05);
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
	deserializeJSON(buffer[name], "unk_05", value.unk_05);
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
const float ReplayFile::cPlayerTiltScale = 1.f;
const float ReplayFile::cData567Scale = 256.f;
const float ReplayFile::cData8Scale = 1.f / 127.f;
const float ReplayFile::cStageTiltScale = 1.f;

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

int main(int argc, char **argv)
{
	using namespace nlohmann;

	std::vector<std::string> args;
	for (int i = 0; i < argc; ++i)
	{
		args.emplace_back(argv[i]);
	}

	if (args.size() < 1)
	{
		//printf("usage: smb-build-replay <JSON configuration file> <output GCI file>");
		return -1;
	}

	//auto inputData = loadFile(args[1]);
	//json inputJSON = json::parse(inputData);

	auto inputData = loadFile(args[1]);
	ReplayFile inputFile;
	deserializeBinary(inputData, inputFile);
	nlohmann::json json;
	serializeJSON(json, "root", inputFile);

	saveFile(args[1] + ".json", stringToBuffer(json.dump(2)));

	ReplayFile outputFile;
	deserializeJSON(json, "root", outputFile);
	std::vector<uint8_t> outputData;
	serializeBinary(outputData, outputFile);
	saveFile(args[1] + ".re", outputData);

	return 0;
}
