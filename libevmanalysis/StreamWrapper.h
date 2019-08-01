#pragma once

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>


namespace
{
struct nop
{
    template <typename T>
    void operator()(T const&) const noexcept
    {}
};

inline bool isGz(std::string filepath)
{
    return filepath.size() >= 3 && filepath.substr(filepath.size() - 3, 3) == ".gz";
}

}

namespace dev
{

template <typename StreamType, typename FileStreamType, typename FilterStreamType>
class StreamWrapper
{
public:
    explicit StreamWrapper(
        const std::string& filepath, std::ios_base::openmode openMode)
            : m_filepath(filepath), m_openMode(openMode) {}

    StreamType& getStream()
    {
        if (m_streamPtr == nullptr)
        {
            init();
        }
        return *m_streamPtr;
    }

protected:
    virtual StreamType& getStdStream() = 0;
    virtual std::ios_base::openmode getOpenMode() = 0;
    virtual void addGzipFilter(std::shared_ptr<FilterStreamType> buf) = 0;

private:
    std::shared_ptr<StreamType> m_streamPtr = nullptr;
    std::shared_ptr<FileStreamType> m_filePtr = nullptr;
    std::shared_ptr<FilterStreamType> m_streamBuf = nullptr;

    void init();
    std::string m_filepath;
    std::ios_base::openmode m_openMode;
};

template <typename StreamType, typename FileStreamType, typename FilterStreamType>
void StreamWrapper<StreamType, FileStreamType, FilterStreamType>::init()
{
    if (m_filepath == "-")
    {
        m_streamPtr = std::shared_ptr<StreamType>(&getStdStream(), nop());
    }
    else
    {
        auto flags = m_openMode | getOpenMode();
        auto isGzip = isGz(m_filepath);
        if (isGzip)
        {
            flags |= std::ios_base::binary;
        }
        m_filePtr = std::make_shared<FileStreamType>(m_filepath, flags);
        if (isGzip)
        {
            m_streamBuf = std::make_shared<FilterStreamType>();
            addGzipFilter(m_streamBuf);
            m_streamBuf->push(*m_filePtr);
            m_streamPtr = std::make_shared<StreamType>(m_streamBuf.get());
        }
        else
        {
            m_streamPtr = m_filePtr;
        }
    }
}


class OStreamWrapper : public StreamWrapper<std::ostream, std::ofstream, boost::iostreams::filtering_ostreambuf>
{
public:
    explicit OStreamWrapper(
        const std::string& filepath, std::ios_base::openmode openMode = std::ios_base::app)
            : StreamWrapper(filepath, openMode) {}

protected:
    virtual std::ostream& getStdStream() override { return std::cout; }
    virtual std::ios_base::openmode getOpenMode() override { return std::ios_base::out; }
    virtual void addGzipFilter(std::shared_ptr<boost::iostreams::filtering_ostreambuf> buf) override
    {
        buf->push(boost::iostreams::gzip_compressor());
    }
};

class IStreamWrapper : public StreamWrapper<std::istream, std::ifstream, boost::iostreams::filtering_istreambuf>
{
public:
    explicit IStreamWrapper(
        const std::string& filepath, std::ios_base::openmode openMode = std::ios_base::in)
            : StreamWrapper(filepath, openMode) {}

protected:
    virtual std::istream& getStdStream() override { return std::cin; }
    virtual std::ios_base::openmode getOpenMode() override { return std::ios_base::in; }
    virtual void addGzipFilter(std::shared_ptr<boost::iostreams::filtering_istreambuf> buf) override
    {
        buf->push(boost::iostreams::gzip_decompressor());
    }
};


}  // namespace dev
