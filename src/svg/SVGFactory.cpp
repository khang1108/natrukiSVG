

#include "SVGFactory.h"

#include "SVGFactoryImpl.h"

#include <memory>

std::unique_ptr<SVGFactory> SVGFactory::createDefaultFactory()
{

  return std::make_unique<SVGFactoryImpl>();
}