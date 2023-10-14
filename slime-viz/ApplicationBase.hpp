#ifndef APPLICATION_BASE_HPP
#define APPLICATION_BASE_HPP

#include "expected.hpp"

using namespace nonstd;

class ApplicationBase {
public:
	virtual void setupTextures() = 0;
	virtual void setupSSBO() = 0;
	virtual expected<void, std::string> setupShaders() = 0;
	virtual void run() = 0;
};

#endif