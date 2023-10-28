#include "filesystem.h"
#include "common/logger.h"

namespace aph
{

Filesystem::Filesystem()
{
    m_protocols["asset"]        = "assets/";
    m_protocols["model"]        = "assets/models";
    m_protocols["font"]         = "assets/fonts";
    m_protocols["shader_glsl"]  = "assets/shaders/glsl";
    m_protocols["shader_slang"] = "assets/shaders/slang";
    m_protocols["texture"]      = "assets/textures";
    m_protocols["file"]         = "";
}

Filesystem::~Filesystem()
{
    clearMappedFiles();
}

void Filesystem::registerProtocol(std::string_view protocol, const std::string& path)
{
    if(protocolExists(protocol))
    {
        CM_LOG_WARN("overrided the existing protocol %s. path: %s -> %s", protocol, m_protocols[protocol.data()], path);
    }
    m_protocols[protocol.data()] = path;
}

bool Filesystem::protocolExists(std::string_view protocol)
{
    return m_protocols.contains(protocol.data());
}

void Filesystem::removeProtocol(std::string_view protocol)
{
    m_protocols.erase(protocol.data());
}

void Filesystem::clearMappedFiles()
{
    for(auto& [data, size] : m_mappedFiles)
    {
        munmap(data, size);
    }
    m_mappedFiles.clear();
}

std::filesystem::path Filesystem::resolvePath(std::string_view inputPath)
{
    auto        protocolEnd = inputPath.find("://");
    std::string protocol;
    std::string relativePath;

    if(protocolEnd != std::string::npos)
    {
        protocol     = inputPath.substr(0, protocolEnd);
        relativePath = inputPath.substr(protocolEnd + 3);
    }
    else
    {
        protocol     = "file";
        relativePath = inputPath;
    }

    if(!m_protocols.contains(protocol))
    {
        CM_LOG_ERR("Unknown protocol: %s", protocol);
        return {};
    }

    return std::filesystem::path(m_protocols[protocol]) / relativePath;
}

void* Filesystem::map(std::string_view path)
{
    std::lock_guard<std::mutex> lock(m_mapLock);
    auto                        resolvedPath = resolvePath(path);

    int fd = open(resolvedPath.string().c_str(), O_RDONLY);
    if(fd == -1)
        return nullptr;

    struct stat fileStat;
    if(fstat(fd, &fileStat) == -1)
    {
        close(fd);
        return nullptr;
    }

    void* mappedData = mmap(0, fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if(mappedData == MAP_FAILED)
    {
        return nullptr;
    }

    m_mappedFiles[mappedData] = fileStat.st_size;
    return mappedData;
}

void Filesystem::unmap(void* data)
{
    if(m_mappedFiles.find(data) != m_mappedFiles.end())
    {
        munmap(data, m_mappedFiles[data]);
        m_mappedFiles.erase(data);
    }
}

std::string Filesystem::readFileToString(std::string_view path)
{
    std::ifstream file(resolvePath(path), std::ios::in);
    if(!file)
    {
        CM_LOG_ERR("Unable to open file: %s", path);
        return {};
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}
std::vector<uint8_t> Filesystem::readFileToBytes(std::string_view path)
{
    std::ifstream file(resolvePath(path), std::ios::binary | std::ios::ate);
    if(!file)
    {
        CM_LOG_ERR("Unable to open file: %s", path);
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if(!file.read((char*)buffer.data(), size))
    {
        CM_LOG_ERR("Error reading file: %s", path);
        return {};
    }
    return buffer;
}
std::vector<std::string> Filesystem::readFileLines(std::string_view path)
{
    std::ifstream file(resolvePath(path));
    if(!file)
    {
        CM_LOG_ERR("Unable to open file: %s", path);
        return {};
    }

    std::vector<std::string> lines;
    std::string              line;
    while(std::getline(file, line))
    {
        lines.push_back(line);
    }
    return lines;
}
void Filesystem::writeStringToFile(std::string_view path, const std::string& content)
{
    std::ofstream file(resolvePath(path).string(), std::ios::binary);
    if(!file)
    {
        throw std::runtime_error("Failed to open file for writing: " + std::string(path));
    }
    file << content;
}
void Filesystem::writeBytesToFile(std::string_view path, const std::vector<uint8_t>& bytes)
{
    std::ofstream file(resolvePath(path).string(), std::ios::binary);
    if(!file)
    {
        throw std::runtime_error("Failed to open file for writing: " + std::string(path));
    }
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
void Filesystem::writeLinesToFile(std::string_view path, const std::vector<std::string>& lines)
{
    std::ofstream file(resolvePath(path).string());
    if(!file)
    {
        throw std::runtime_error("Failed to open file for writing: " + std::string(path));
    }
    for(const auto& line : lines)
    {
        file << line << '\n';
    }
}
}  // namespace aph
