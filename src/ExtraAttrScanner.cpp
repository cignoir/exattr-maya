#include "ExtraAttrScanner.h"
#include <maya/MItDependencyNodes.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MPlug.h>
#include <maya/MGlobal.h>

ExtraAttrScanner::ExtraAttrScanner()
    : m_totalNodesScanned(0)
{
}

ExtraAttrScanner::~ExtraAttrScanner()
{
    clearCache();
}

bool ExtraAttrScanner::scanScene()
{
    clearCache();

    MStatus status;
    MItDependencyNodes nodeIt(MFn::kInvalid, &status);
    if (status != MS::kSuccess) {
        MGlobal::displayError("Failed to create node iterator");
        return false;
    }

    // Iterate through all nodes
    for (; !nodeIt.isDone(); nodeIt.next()) {
        MObject node = nodeIt.thisNode(&status);
        if (status != MS::kSuccess) {
            continue;
        }

        scanNode(node);
        m_totalNodesScanned++;
    }

    MGlobal::displayInfo(MString("Scan complete. Found ") +
                         m_attributeInfoMap.size() +
                         " extra attributes in " +
                         m_totalNodesScanned + " nodes.");

    return true;
}

void ExtraAttrScanner::scanNode(const MObject& depNode)
{
    if (depNode.isNull()) {
        return;
    }

    MStatus status;
    MFnDependencyNode fnDep(depNode, &status);
    if (status != MS::kSuccess) {
        return;
    }

    // Iterate through all attributes
    unsigned int attrCount = fnDep.attributeCount(&status);
    if (status != MS::kSuccess) {
        return;
    }

    for (unsigned int i = 0; i < attrCount; ++i) {
        MObject attr = fnDep.attribute(i, &status);
        if (status != MS::kSuccess || attr.isNull()) {
            continue;
        }

        // Check if it's an Extra Attribute
        if (!isExtraAttribute(attr, fnDep)) {
            continue;
        }

        // Get attribute information
        MFnAttribute fnAttr(attr, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        MString attrName = fnAttr.name(&status);
        if (status != MS::kSuccess || attrName.length() == 0) {
            continue;
        }

        // Update existing attribute information or create new one
        auto it = m_attributeInfoMap.find(attrName);
        if (it == m_attributeInfoMap.end()) {
            // Found new Extra Attribute
            auto attrInfo = std::make_shared<AttributeInfo>();
            attrInfo->name = attrName;
            attrInfo->typeName = getAttributeTypeName(attr);
            attrInfo->apiType = attr.apiType();
            attrInfo->isArray = fnAttr.isArray(&status);
            attrInfo->usageCount = 1;

            m_attributeInfoMap[attrName] = attrInfo;
            m_attrToNodesMap[attrName] = std::vector<MObject>();
            m_attrToNodesMap[attrName].push_back(depNode);
        } else {
            // Increase usage count for existing Extra Attribute
            it->second->usageCount++;
            m_attrToNodesMap[attrName].push_back(depNode);
        }
    }
}

bool ExtraAttrScanner::isExtraAttribute(const MObject& attr, const MFnDependencyNode& fnDep) const
{
    if (attr.isNull()) {
        return false;
    }

    MStatus status;
    MFnAttribute fnAttr(attr, &status);
    if (status != MS::kSuccess) {
        return false;
    }

    // Check if it's a dynamic attribute (user-added)
    // If isDynamic() is true, it's an attribute added by the user or script
    bool isDynamic = fnAttr.isDynamic(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    // Consider only dynamic attributes as Extra Attributes
    return isDynamic;
}

MString ExtraAttrScanner::getAttributeTypeName(const MObject& attr) const
{
    if (attr.isNull()) {
        return "unknown";
    }

    MFn::Type apiType = attr.apiType();

    switch (apiType) {
        case MFn::kNumericAttribute: {
            MFnNumericAttribute fnNum(attr);
            MFnNumericData::Type numType = fnNum.unitType();
            switch (numType) {
                case MFnNumericData::kBoolean: return "bool";
                case MFnNumericData::kInt: return "int";
                case MFnNumericData::kFloat: return "float";
                case MFnNumericData::kDouble: return "double";
                case MFnNumericData::kShort: return "short";
                // kLong is commented out because it has the same value as kInt
                // case MFnNumericData::kLong: return "long";
                case MFnNumericData::k2Float: return "float2";
                case MFnNumericData::k2Double: return "double2";
                case MFnNumericData::k3Float: return "float3";
                case MFnNumericData::k3Double: return "double3";
                default: return "numeric";
            }
        }
        case MFn::kTypedAttribute: {
            MFnTypedAttribute fnTyped(attr);
            MFnData::Type dataType = fnTyped.attrType();
            switch (dataType) {
                case MFnData::kString: return "string";
                case MFnData::kMatrix: return "matrix";
                case MFnData::kStringArray: return "stringArray";
                case MFnData::kDoubleArray: return "doubleArray";
                case MFnData::kIntArray: return "intArray";
                case MFnData::kPointArray: return "pointArray";
                case MFnData::kVectorArray: return "vectorArray";
                default: return "typed";
            }
        }
        case MFn::kEnumAttribute: return "enum";
        case MFn::kMessageAttribute: return "message";
        case MFn::kCompoundAttribute: return "compound";
        case MFn::kUnitAttribute: {
            MFnUnitAttribute fnUnit(attr);
            MFnUnitAttribute::Type unitType = fnUnit.unitType();
            switch (unitType) {
                case MFnUnitAttribute::kAngle: return "angle";
                case MFnUnitAttribute::kDistance: return "distance";
                case MFnUnitAttribute::kTime: return "time";
                default: return "unit";
            }
        }
        default:
            return "unknown";
    }
}

MString ExtraAttrScanner::getAttributeValueAsString(const MObject& node, const MObject& attr) const
{
    if (node.isNull() || attr.isNull()) {
        return "";
    }

    MStatus status;
    MFnDependencyNode fnDep(node, &status);
    if (status != MS::kSuccess) {
        return "";
    }

    MFnAttribute fnAttr(attr, &status);
    if (status != MS::kSuccess) {
        return "";
    }

    MString attrName = fnAttr.name(&status);
    if (status != MS::kSuccess) {
        return "";
    }

    MPlug plug = fnDep.findPlug(attr, &status);
    if (status != MS::kSuccess) {
        return "";
    }

    // Get value according to attribute type
    MFn::Type apiType = attr.apiType();

    switch (apiType) {
        case MFn::kNumericAttribute: {
            MFnNumericAttribute fnNum(attr);
            MFnNumericData::Type numType = fnNum.unitType();
            switch (numType) {
                case MFnNumericData::kBoolean: {
                    bool value;
                    plug.getValue(value);
                    return value ? "true" : "false";
                }
                case MFnNumericData::kInt: {
                    int value;
                    plug.getValue(value);
                    return MString() + value;
                }
                case MFnNumericData::kFloat: {
                    float value;
                    plug.getValue(value);
                    return MString() + value;
                }
                case MFnNumericData::kDouble: {
                    double value;
                    plug.getValue(value);
                    return MString() + value;
                }
                case MFnNumericData::kShort: {
                    short value;
                    plug.getValue(value);
                    return MString() + value;
                }
                default:
                    return plug.asString(&status);
            }
        }
        case MFn::kTypedAttribute: {
            MFnTypedAttribute fnTyped(attr);
            MFnData::Type dataType = fnTyped.attrType();
            if (dataType == MFnData::kString) {
                MString value;
                plug.getValue(value);
                return value;
            }
            return plug.asString(&status);
        }
        case MFn::kEnumAttribute: {
            MFnEnumAttribute fnEnum(attr);
            short value;
            plug.getValue(value);

            // Convert enum value to string name
            MString fieldName = fnEnum.fieldName(value, &status);
            if (status == MS::kSuccess && fieldName.length() > 0) {
                return fieldName;
            }
            // Return numeric value if field name cannot be obtained
            return MString() + value;
        }
        default:
            return plug.asString(&status);
    }
}

const std::map<MString, std::shared_ptr<AttributeInfo>, MStringLess>&
ExtraAttrScanner::getAttributeInfoMap() const
{
    return m_attributeInfoMap;
}

std::vector<NodeAttributeValue> ExtraAttrScanner::getNodesWithAttribute(const MString& attrName) const
{
    std::vector<NodeAttributeValue> result;

    auto it = m_attrToNodesMap.find(attrName);
    if (it == m_attrToNodesMap.end()) {
        return result;
    }

    // Get attribute information
    auto attrIt = m_attributeInfoMap.find(attrName);
    if (attrIt == m_attributeInfoMap.end()) {
        return result;
    }

    // Collect information for each node
    MStatus status;
    for (const MObject& node : it->second) {
        if (node.isNull()) {
            continue;
        }

        MFnDependencyNode fnDep(node, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        NodeAttributeValue nodeValue;
        nodeValue.nodeObj = node;
        nodeValue.nodeName = fnDep.name(&status);
        nodeValue.nodeType = fnDep.typeName(&status);

        // Get attribute value
        MObject attr = fnDep.attribute(attrName, &status);
        if (status == MS::kSuccess && !attr.isNull()) {
            nodeValue.valueStr = getAttributeValueAsString(node, attr);
        }

        result.push_back(nodeValue);
    }

    return result;
}

void ExtraAttrScanner::clearCache()
{
    m_attributeInfoMap.clear();
    m_attrToNodesMap.clear();
    m_totalNodesScanned = 0;
}

void ExtraAttrScanner::getStatistics(int& totalAttrs, int& totalNodes) const
{
    totalAttrs = static_cast<int>(m_attributeInfoMap.size());
    totalNodes = m_totalNodesScanned;
}
