#ifndef EXTRA_ATTR_SCANNER_H
#define EXTRA_ATTR_SCANNER_H

#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MFnAttribute.h>
#include <vector>
#include <map>
#include <memory>

// Structure to store Extra Attribute information
struct AttributeInfo {
    MString name;              // Attribute name
    MString typeName;          // Type name (e.g., "double", "string", "bool")
    MFn::Type apiType;         // Maya API type
    int usageCount;            // Number of nodes using this attribute
    bool isArray;              // Whether it's an array attribute

    AttributeInfo()
        : name(""), typeName(""), apiType(MFn::kInvalid),
          usageCount(0), isArray(false) {}
};

// Functor for MString comparison
struct MStringLess {
    bool operator()(const MString& a, const MString& b) const {
        return strcmp(a.asChar(), b.asChar()) < 0;
    }
};

// Node and attribute value pair
struct NodeAttributeValue {
    MString nodeName;          // Node name
    MString nodeType;          // Node type
    MObject nodeObj;           // Node object
    MString valueStr;          // Attribute value (string representation)

    NodeAttributeValue()
        : nodeName(""), nodeType(""), nodeObj(MObject::kNullObj), valueStr("") {}
};

/**
 * @class ExtraAttrScanner
 * @brief Class to scan and collect information about Extra Attributes in the scene
 */
class ExtraAttrScanner {
public:
    ExtraAttrScanner();
    ~ExtraAttrScanner();

    /**
     * @brief Scan the entire scene to detect Extra Attributes
     * @return true on success
     */
    bool scanScene();

    /**
     * @brief Get the list of detected Extra Attributes
     * @return Map of attribute information keyed by attribute name
     */
    const std::map<MString, std::shared_ptr<AttributeInfo>, MStringLess>& getAttributeInfoMap() const;

    /**
     * @brief Get the list of nodes using the specified attribute and their values
     * @param attrName Attribute name
     * @return List of nodes and attribute values
     */
    std::vector<NodeAttributeValue> getNodesWithAttribute(const MString& attrName) const;

    /**
     * @brief Clear cache
     */
    void clearCache();

    /**
     * @brief Get scan result statistics
     * @param totalAttrs Total number of Extra Attributes
     * @param totalNodes Number of scanned nodes
     */
    void getStatistics(int& totalAttrs, int& totalNodes) const;

private:
    /**
     * @brief Check if a node has Extra Attributes
     * @param depNode Dependency node
     */
    void scanNode(const MObject& depNode);

    /**
     * @brief Determine if an attribute is an Extra Attribute (excluding default attributes)
     * @param attr Attribute object
     * @param fnDep Dependency node function set
     * @return true if it's an Extra Attribute
     */
    bool isExtraAttribute(const MObject& attr, const class MFnDependencyNode& fnDep) const;

    /**
     * @brief Get attribute value as string representation
     * @param node Node object
     * @param attr Attribute object
     * @return String representation of attribute value
     */
    MString getAttributeValueAsString(const MObject& node, const MObject& attr) const;

    /**
     * @brief Get attribute type name
     * @param attr Attribute object
     * @return Type name
     */
    MString getAttributeTypeName(const MObject& attr) const;

private:
    // Map of attribute name -> attribute information
    std::map<MString, std::shared_ptr<AttributeInfo>, MStringLess> m_attributeInfoMap;

    // Map of attribute name -> node list (cache)
    std::map<MString, std::vector<MObject>, MStringLess> m_attrToNodesMap;

    // Total number of scanned nodes
    int m_totalNodesScanned;
};

#endif // EXTRA_ATTR_SCANNER_H
