#include "StreamWrapper.h"

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <iostream>


namespace
{
bool isGz(std::string filepath)
{
    return filepath.size() >= 3 && filepath.substr(filepath.size() - 3, 3) == ".gz";
}

struct nop
{
    template <typename T>
    void operator()(T const&) const noexcept
    {}
};

}  // namespace

namespace dev
{
StreamWrapper::StreamWrapper(const std::string& filepath, std::ios_base::openmode openMode)
{
    if (filepath == "-")
    {
        m_streamPtr = std::shared_ptr<std::ostream>(&std::cout, nop());
    }
    else
    {
        auto flags = openMode | std::ios_base::out;
        if (isGz(filepath))
        {
            flags |= std::ios_base::binary;
        }
        m_filePtr = std::make_shared<std::ofstream>(filepath, flags);
        if (isGz(filepath))
        {
            m_ostreamBuf = std::make_shared<boost::iostreams::filtering_ostreambuf>();
            m_ostreamBuf->push(boost::iostreams::gzip_compressor());
            m_ostreamBuf->push(*m_filePtr);
            m_streamPtr = std::make_shared<std::ostream>(m_ostreamBuf.get());
        }
        else
        {
            m_streamPtr = m_filePtr;
        }
    }
}

}  // namespace dev
