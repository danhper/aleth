#include "StreamWrapper.h"

#include <iostream>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>


namespace
{

bool isGz(std::string filepath)
{ 
    return filepath.size() >= 3 && filepath.substr(filepath.size() - 3, 3) == ".gz";
}

struct nop {
    template <typename T>
    void operator() (T const &) const noexcept { }
};

}

namespace dev
{

StreamWrapper::StreamWrapper(const std::string& filepath, std::ios_base::openmode openMode)
{
    std::shared_ptr<boost::iostreams::filtering_ostreambuf> ostreamBuf;
    if (filepath == "-") {
        m_streamPtr = std::shared_ptr<std::ostream>(&std::cout, nop());
    } else {
        auto flags = std::ios_base::out;
        if (openMode > 0) {
            flags |= static_cast<std::ios_base::openmode>(openMode);
        }
        if (isGz(filepath)) {
            flags |= std::ios_base::binary;
        }
        m_filePtr = std::make_shared<std::ofstream>(filepath, flags);
        if (isGz(filepath)) {
            m_ostreamBuf = std::make_shared<boost::iostreams::filtering_ostreambuf>();
            m_ostreamBuf->push(boost::iostreams::gzip_compressor());
            m_ostreamBuf->push(*m_filePtr);
            m_streamPtr = std::make_shared<std::ostream>(ostreamBuf.get());
        } else {
            m_streamPtr = m_filePtr;
        }
    }
}

}
