#ifndef MAYA_UTILS_H
#define MAYA_UTILS_H

#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

/**
 * @brief Utility functions for Maya API operations
 */
namespace MayaUtils {

/**
 * @brief Get a dependency node from its name
 * @param nodeName Name of the node
 * @param outNode Output MObject for the node
 * @return true on success, false on failure
 */
bool getNodeFromName(const MString& nodeName, MObject& outNode);

/**
 * @brief Get a dependency node function set from node name
 * @param nodeName Name of the node
 * @param outFnDep Output MFnDependencyNode
 * @return true on success, false on failure
 */
bool getDependencyNodeFromName(const MString& nodeName, MFnDependencyNode& outFnDep);

/**
 * @brief Set attribute value from string representation
 * @param plug MPlug to set value on
 * @param attr MObject of the attribute
 * @param value String representation of the value
 * @return true on success, false on failure
 */
bool setAttributeValueFromString(MPlug& plug, const MObject& attr, const MString& value);

} // namespace MayaUtils

#endif // MAYA_UTILS_H
