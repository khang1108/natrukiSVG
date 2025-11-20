

#include "SVGFactory.h"

#include "SVGFactoryImpl.h"

#include <memory>

/**
 * @brief Creates the default factory implementation for parsing SVG elements.
 *
 * Algorithm:
 * - Factory Pattern: Creates a concrete implementation of the abstract SVGFactory
 * - Returns a unique_ptr to SVGFactoryImpl, which handles actual element creation
 * - This allows the factory implementation to be swapped if needed
 *
 * @return Unique pointer to the default factory implementation
 */
std::unique_ptr<SVGFactory> SVGFactory::createDefaultFactory()
{

    return std::make_unique<SVGFactoryImpl>();
}